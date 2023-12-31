/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/cpu/linux.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/efi/uga_draw.h>
#include <grub/efi/graphics_output.h>
#include <grub/pci.h>
#include <grub/command.h>
#include <grub/memory.h>
#include <grub/i18n.h>

#define GRUB_LINUX_CL_OFFSET		0x1000
#define GRUB_LINUX_CL_END_OFFSET	0x2000

#define NEXT_MEMORY_DESCRIPTOR(desc, size)      \
  ((grub_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

static grub_dl_t my_mod;

static grub_size_t linux_mem_size;
static int loaded;
static void *real_mode_mem;
static void *prot_mode_mem;
static void *initrd_mem;
static grub_efi_uintn_t real_mode_pages;
static grub_efi_uintn_t prot_mode_pages;
static grub_efi_uintn_t initrd_pages;
static void *mmap_buf;

static grub_uint8_t gdt[] __attribute__ ((aligned(16))) =
  {
    /* NULL.  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Reserved.  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Code segment.  */
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00,
    /* Data segment.  */
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
  };

struct gdt_descriptor
{
  grub_uint16_t limit;
  void *base;
} __attribute__ ((packed));

static struct gdt_descriptor gdt_desc =
  {
    sizeof (gdt) - 1,
    gdt
  };

struct idt_descriptor
{
  grub_uint16_t limit;
  void *base;
} __attribute__ ((packed));

static struct idt_descriptor idt_desc =
  {
    0,
    0
  };

static inline grub_size_t
page_align (grub_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static grub_efi_uintn_t
find_mmap_size (void)
{
  static grub_efi_uintn_t mmap_size = 0;

  if (mmap_size != 0)
    return mmap_size;

  mmap_size = (1 << 12);
  while (1)
    {
      int ret;
      grub_efi_memory_descriptor_t *mmap;
      grub_efi_uintn_t desc_size;

      mmap = grub_malloc (mmap_size);
      if (! mmap)
	return 0;

      ret = grub_efi_get_memory_map (&mmap_size, mmap, 0, &desc_size, 0);
      grub_free (mmap);

      if (ret < 0)
	grub_fatal ("cannot get memory map");
      else if (ret > 0)
	break;

      mmap_size += (1 << 12);
    }

  /* Increase the size a bit for safety, because GRUB allocates more on
     later, and EFI itself may allocate more.  */
  mmap_size += (1 << 12);

  return page_align (mmap_size);
}

static void
free_pages (void)
{
  if (real_mode_mem)
    {
      grub_efi_free_pages ((grub_addr_t) real_mode_mem, real_mode_pages);
      real_mode_mem = 0;
    }

  if (prot_mode_mem)
    {
      grub_efi_free_pages ((grub_addr_t) prot_mode_mem, prot_mode_pages);
      prot_mode_mem = 0;
    }

  if (initrd_mem)
    {
      grub_efi_free_pages ((grub_addr_t) initrd_mem, initrd_pages);
      initrd_mem = 0;
    }
}

/* Allocate pages for the real mode code and the protected mode code
   for linux as well as a memory map buffer.  */
static int
allocate_pages (grub_size_t prot_size)
{
  grub_efi_uintn_t desc_size;
  grub_efi_memory_descriptor_t *mmap, *mmap_end;
  grub_efi_uintn_t mmap_size, tmp_mmap_size;
  grub_efi_memory_descriptor_t *desc;
  grub_size_t real_size;

  /* Make sure that each size is aligned to a page boundary.  */
  real_size = GRUB_LINUX_CL_END_OFFSET;
  prot_size = page_align (prot_size);
  mmap_size = find_mmap_size ();

  grub_dprintf ("linux", "real_size = %x, prot_size = %x, mmap_size = %x\n",
		(unsigned) real_size, (unsigned) prot_size, (unsigned) mmap_size);

  /* Calculate the number of pages; Combine the real mode code with
     the memory map buffer for simplicity.  */
  real_mode_pages = ((real_size + mmap_size) >> 12);
  prot_mode_pages = (prot_size >> 12);

  /* Initialize the memory pointers with NULL for convenience.  */
  real_mode_mem = 0;
  prot_mode_mem = 0;

  /* Read the memory map temporarily, to find free space.  */
  mmap = grub_malloc (mmap_size);
  if (! mmap)
    return 0;

  tmp_mmap_size = mmap_size;
  if (grub_efi_get_memory_map (&tmp_mmap_size, mmap, 0, &desc_size, 0) <= 0)
    grub_fatal ("cannot get memory map");

  mmap_end = NEXT_MEMORY_DESCRIPTOR (mmap, tmp_mmap_size);

  /* First, find free pages for the real mode code
     and the memory map buffer.  */
  for (desc = mmap;
       desc < mmap_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      /* Probably it is better to put the real mode code in the traditional
	 space for safety.  */
      if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY
	  && desc->physical_start <= 0x90000
	  && desc->num_pages >= real_mode_pages)
	{
	  grub_efi_physical_address_t physical_end;
	  grub_efi_physical_address_t addr;

	  physical_end = desc->physical_start + (desc->num_pages << 12);
	  if (physical_end > 0x90000)
	    physical_end = 0x90000;

	  grub_dprintf ("linux", "physical_start = %x, physical_end = %x\n",
			(unsigned) desc->physical_start,
			(unsigned) physical_end);
	  addr = physical_end - real_size - mmap_size;
	  if (addr < 0x10000)
	    continue;

	  grub_dprintf ("linux", "trying to allocate %u pages at %lx\n",
			(unsigned) real_mode_pages, (unsigned long) addr);
	  real_mode_mem = grub_efi_allocate_pages (addr, real_mode_pages);
	  if (! real_mode_mem)
	    grub_fatal ("cannot allocate pages");

	  desc->num_pages -= real_mode_pages;
	  break;
	}
    }

  if (! real_mode_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "cannot allocate real mode pages");
      goto fail;
    }

  mmap_buf = (void *) ((char *) real_mode_mem + real_size);

  /* Next, find free pages for the protected mode code.  */
  /* XXX what happens if anything is using this address?  */
  prot_mode_mem = grub_efi_allocate_pages (0x100000, prot_mode_pages + 1);
  if (! prot_mode_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "cannot allocate protected mode pages");
      goto fail;
    }

  grub_dprintf ("linux", "real_mode_mem = %lx, real_mode_pages = %x, "
		"prot_mode_mem = %lx, prot_mode_pages = %x\n",
		(unsigned long) real_mode_mem, (unsigned) real_mode_pages,
		(unsigned long) prot_mode_mem, (unsigned) prot_mode_pages);

  grub_free (mmap);
  return 1;

 fail:
  grub_free (mmap);
  free_pages ();
  return 0;
}

static void
grub_e820_add_region (struct grub_e820_mmap *e820_map, int *e820_num,
		      grub_uint64_t start, grub_uint64_t size,
		      grub_uint32_t type)
{
  int n = *e820_num;

  if (n >= GRUB_E820_MAX_ENTRY)
    grub_fatal ("Too many e820 memory map entries");

  if ((n > 0) && (e820_map[n - 1].addr + e820_map[n - 1].size == start) &&
      (e820_map[n - 1].type == type))
      e820_map[n - 1].size += size;
  else
    {
      e820_map[n].addr = start;
      e820_map[n].size = size;
      e820_map[n].type = type;
      (*e820_num)++;
    }
}

#ifdef __x86_64__
extern grub_uint8_t grub_linux_trampoline_start[];
extern grub_uint8_t grub_linux_trampoline_end[];
#endif

struct grub_linux_boot_closure
{
  struct linux_kernel_params *params;
  int e820_num;
};

static int
grub_linux_boot_hook (grub_uint64_t addr, grub_uint64_t size,
		      grub_uint32_t type, void *closure)
{
  struct grub_linux_boot_closure *c = closure;

  switch (type)
    {
    case GRUB_MACHINE_MEMORY_AVAILABLE:
      grub_e820_add_region (c->params->e820_map, &c->e820_num,
			    addr, size, GRUB_E820_RAM);
      break;

#ifdef GRUB_MACHINE_MEMORY_ACPI
    case GRUB_MACHINE_MEMORY_ACPI:
      grub_e820_add_region (c->params->e820_map, &c->e820_num,
			    addr, size, GRUB_E820_ACPI);
      break;
#endif

#ifdef GRUB_MACHINE_MEMORY_NVS
    case GRUB_MACHINE_MEMORY_NVS:
      grub_e820_add_region (c->params->e820_map, &c->e820_num,
			    addr, size, GRUB_E820_NVS);
      break;
#endif

#ifdef GRUB_MACHINE_MEMORY_CODE
    case GRUB_MACHINE_MEMORY_CODE:
      grub_e820_add_region (c->params->e820_map, &c->e820_num,
			    addr, size, GRUB_E820_EXEC_CODE);
      break;
#endif

    default:
      grub_e820_add_region (c->params->e820_map, &c->e820_num,
			    addr, size, GRUB_E820_RESERVED);
    }
  return 0;
}

static grub_err_t
grub_linux_boot (void)
{
  grub_efi_uintn_t mmap_size;
  grub_efi_uintn_t map_key;
  grub_efi_uintn_t desc_size;
  grub_efi_uint32_t desc_version;
  struct grub_linux_boot_closure c;

  c.params = real_mode_mem;

  grub_dprintf ("linux", "code32_start = %x, idt_desc = %lx, gdt_desc = %lx\n",
		(unsigned) c.params->code32_start,
		(unsigned long) &(idt_desc.limit),
		(unsigned long) &(gdt_desc.limit));
  grub_dprintf ("linux", "idt = %x:%lx, gdt = %x:%lx\n",
		(unsigned) idt_desc.limit, (unsigned long) idt_desc.base,
		(unsigned) gdt_desc.limit, (unsigned long) gdt_desc.base);

  c.e820_num = 0;
  grub_mmap_iterate (grub_linux_boot_hook, &c);
  c.params->mmap_size = c.e820_num;

  mmap_size = find_mmap_size ();
  if (grub_efi_get_memory_map (&mmap_size, mmap_buf, &map_key,
			       &desc_size, &desc_version) <= 0)
    grub_fatal ("cannot get memory map");

  if (! grub_efi_exit_boot_services (map_key))
     grub_fatal ("cannot exit boot services");

  /* Note that no boot services are available from here.  */

  /* Pass EFI parameters.  */
  if (grub_le_to_cpu16 (c.params->version) >= 0x0206)
    {
      c.params->v0206.efi_mem_desc_size = desc_size;
      c.params->v0206.efi_mem_desc_version = desc_version;
      c.params->v0206.efi_mmap = (grub_uint32_t) (unsigned long) mmap_buf;
      c.params->v0206.efi_mmap_size = mmap_size;
#ifdef __x86_64__
      c.params->v0206.efi_mmap_hi = (grub_uint32_t) ((grub_uint64_t) mmap_buf >> 32);
#endif
    }
  else if (grub_le_to_cpu16 (c.params->version) >= 0x0204)
    {
      c.params->v0204.efi_mem_desc_size = desc_size;
      c.params->v0204.efi_mem_desc_version = desc_version;
      c.params->v0204.efi_mmap = (grub_uint32_t) (unsigned long) mmap_buf;
      c.params->v0204.efi_mmap_size = mmap_size;
    }

#ifdef __x86_64__

  grub_memcpy ((char *) prot_mode_mem + (prot_mode_pages << 12),
	       grub_linux_trampoline_start,
	       grub_linux_trampoline_end - grub_linux_trampoline_start);

  ((void (*) (unsigned long, void *)) ((char *) prot_mode_mem
                                     + (prot_mode_pages << 12)))
    (c.params->code32_start, real_mode_mem);

#else

  /* Hardware interrupts are not safe any longer.  */
  asm volatile ("cli" : : );

  /* Load the IDT and the GDT for the bootstrap.  */
  asm volatile ("lidt %0" : : "m" (idt_desc));
  asm volatile ("lgdt %0" : : "m" (gdt_desc));

  /* Pass parameters.  */
  asm volatile ("movl %0, %%ecx" : : "m" (c.params->code32_start));
  asm volatile ("movl %0, %%esi" : : "m" (real_mode_mem));

  asm volatile ("xorl %%ebx, %%ebx" : : );

  /* Enter Linux.  */
  asm volatile ("jmp *%%ecx" : : );

#endif

  /* Never reach here.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_unload (void)
{
  free_pages ();
  grub_dl_unref (my_mod);
  loaded = 0;
  return GRUB_ERR_NONE;
}

static grub_efi_guid_t uga_draw_guid = GRUB_EFI_UGA_DRAW_GUID;
static grub_efi_guid_t graphics_output_guid = GRUB_EFI_GRAPHICS_OUTPUT_GUID;

#define RGB_MASK	0xffffff
#define RGB_MAGIC	0x121314
#define LINE_MIN	800
#define LINE_MAX	4096
#define FBTEST_STEP	(0x10000 >> 2)
#define FBTEST_COUNT	8

static int
find_line_len (grub_uint32_t *fb_base, grub_uint32_t *line_len)
{
  grub_uint32_t *base = (grub_uint32_t *) (grub_target_addr_t) *fb_base;
  int i;

  for (i = 0; i < FBTEST_COUNT; i++, base += FBTEST_STEP)
    {
      if ((*base & RGB_MASK) == RGB_MAGIC)
	{
	  int j;

	  for (j = LINE_MIN; j <= LINE_MAX; j++)
	    {
	      if ((base[j] & RGB_MASK) == RGB_MAGIC)
		{
		  *fb_base = (grub_uint32_t) (grub_target_addr_t) base;
		  *line_len = j << 2;

		  return 1;
		}
	    }

	  break;
	}
    }

  return 0;
}

struct find_framebuf_closure
{
  grub_uint32_t *fb_base;
  grub_uint32_t *line_len;
  int found;
};

static int
find_card (grub_pci_device_t dev, grub_pci_id_t pciid, void *closure)
{
  struct find_framebuf_closure *c = closure;
  grub_pci_address_t addr;

  addr = grub_pci_make_address (dev, GRUB_PCI_REG_CLASS);
  if (grub_pci_read (addr) >> 24 == 0x3)
    {
      int i;

      grub_printf ("Display controller: %d:%d.%d\nDevice id: %x\n",
		   grub_pci_get_bus (dev), grub_pci_get_device (dev),
		   grub_pci_get_function (dev), pciid);
      addr += 8;
      for (i = 0; i < 6; i++, addr += 4)
	{
	  grub_uint32_t old_bar1, old_bar2, type;
	  grub_uint64_t base64;

	  old_bar1 = grub_pci_read (addr);
	  if ((! old_bar1) || (old_bar1 & GRUB_PCI_ADDR_SPACE_IO))
	    continue;

	  type = old_bar1 & GRUB_PCI_ADDR_MEM_TYPE_MASK;
	  if (type == GRUB_PCI_ADDR_MEM_TYPE_64)
	    {
	      if (i == 5)
		break;

	      old_bar2 = grub_pci_read (addr + 4);
	    }
	  else
	    old_bar2 = 0;

	  base64 = old_bar2;
	  base64 <<= 32;
	  base64 |= (old_bar1 & GRUB_PCI_ADDR_MEM_MASK);

	  grub_printf ("%s(%d): 0x%llx\n",
		       ((old_bar1 & GRUB_PCI_ADDR_MEM_PREFETCH) ?
			"VMEM" : "MMIO"), i,
		       (unsigned long long) base64);

	  if ((old_bar1 & GRUB_PCI_ADDR_MEM_PREFETCH) && (! c->found))
	    {
	      *(c->fb_base) = base64;
	      if (find_line_len (c->fb_base, c->line_len))
		c->found++;
	    }

	  if (type == GRUB_PCI_ADDR_MEM_TYPE_64)
	    {
	      i++;
	      addr += 4;
	    }
	}
    }

  return c->found;
}

static int
find_framebuf (grub_uint32_t *fb_base, grub_uint32_t *line_len)
{
  struct find_framebuf_closure c;

  c.fb_base = fb_base;
  c.line_len = line_len;
  c.found = 0;
  grub_pci_iterate (find_card, &c);
  return c.found;
}

static int
grub_linux_setup_video (struct linux_kernel_params *params)
{
  struct grub_efi_graphics_output_protocol *gop;
  grub_uint32_t width, height, depth, fb_base, line_len;
  int ret;

  gop = grub_efi_locate_protocol (&graphics_output_guid, 0);
  if (gop)
    {
      width = gop->mode->info->horizontal_resolution;
      height = gop->mode->info->vertical_resolution;

      if ((gop->mode->frame_buffer_base) && (gop->mode->frame_buffer_size))
	{
	  fb_base = gop->mode->frame_buffer_base;
	  line_len = 4 * gop->mode->info->pixels_per_scan_line;
	}
      else
	{
	  grub_uint32_t pixel;

	  grub_efi_set_text_mode (0);
	  pixel = RGB_MAGIC;
	  efi_call_10 (gop->blt, gop, (struct grub_efi_uga_pixel *) &pixel,
		       GRUB_EFI_UGA_VIDEO_FILL, 0, 0, 0, 0, 1, height, 0);
	  ret = find_framebuf (&fb_base, &line_len);
	  grub_efi_set_text_mode (1);

	  if (! ret)
	    {
	      grub_printf ("Can\'t find frame buffer address\n");
	      return 1;
	    }
	}

      depth = 32;
    }
  else
    {
      grub_efi_uga_draw_protocol_t *c;
      grub_uint32_t rate, pixel;

      c = grub_efi_locate_protocol (&uga_draw_guid, 0);
      if (! c)
	{
	  grub_printf ("Can\'t locate uga protocol\n");
	  return 1;
	}

      if (efi_call_5 (c->get_mode, c, &width, &height, &depth, &rate))
	{
	  grub_printf ("Can\'t get mode\n");
	  return 1;
	}

      grub_efi_set_text_mode (0);
      pixel = RGB_MAGIC;
      efi_call_10 (c->blt, c, (struct grub_efi_uga_pixel *) &pixel,
		   GRUB_EFI_UGA_VIDEO_FILL, 0, 0, 0, 0, 1, height, 0);
      ret = find_framebuf (&fb_base, &line_len);
      grub_efi_set_text_mode (1);

      if (! ret)
	{
	  grub_printf ("Can\'t find frame buffer address\n");
	  return 1;
	}
    }

  grub_printf ("Video mode: %ux%u-%u\n", width, height, depth);
  grub_printf ("Frame buffer base: 0x%x\n", fb_base);
  grub_printf ("Video line length: %d\n", line_len);

  params->lfb_width = width;
  params->lfb_height = height;
  params->lfb_depth = depth;
  params->lfb_line_len = line_len;

  params->lfb_base = fb_base;
  params->lfb_size = (line_len * params->lfb_height + 65535) >> 16;

  params->red_mask_size = 8;
  params->red_field_pos = 16;
  params->green_mask_size = 8;
  params->green_field_pos = 8;
  params->blue_mask_size = 8;
  params->blue_field_pos = 0;
  params->reserved_mask_size = 8;
  params->reserved_field_pos = 24;

  params->have_vga = GRUB_VIDEO_LINUX_TYPE_VESA;
  params->vid_mode = 0x338;  /* 1024x768x32  */

  return 0;
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  struct linux_kernel_header lh;
  struct linux_kernel_params *params;
  grub_uint8_t setup_sects;
  grub_size_t real_size, prot_size;
  grub_ssize_t len;
  int i;
  char *dest;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  if (grub_file_read (file, &lh, sizeof (lh)) != sizeof (lh))
    {
      grub_error (GRUB_ERR_READ_ERROR, "cannot read the Linux header");
      goto fail;
    }

  if (lh.boot_flag != grub_cpu_to_le16 (0xaa55))
    {
      grub_error (GRUB_ERR_BAD_OS, "invalid magic number");
      goto fail;
    }

  if (lh.setup_sects > GRUB_LINUX_MAX_SETUP_SECTS)
    {
      grub_error (GRUB_ERR_BAD_OS, "too many setup sectors");
      goto fail;
    }

  /* EFI support is quite new, so reject old versions.  */
  if (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      || grub_le_to_cpu16 (lh.version) < 0x0203)
    {
      grub_error (GRUB_ERR_BAD_OS, "too old version");
      goto fail;
    }

  /* I'm not sure how to support zImage on EFI.  */
  if (! (lh.loadflags & GRUB_LINUX_FLAG_BIG_KERNEL))
    {
      grub_error (GRUB_ERR_BAD_OS, "zImage is not supported");
      goto fail;
    }

  setup_sects = lh.setup_sects;

  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << GRUB_DISK_SECTOR_BITS;
  prot_size = grub_file_size (file) - real_size - GRUB_DISK_SECTOR_SIZE;

  if (! allocate_pages (prot_size))
    goto fail;

  params = (struct linux_kernel_params *) real_mode_mem;
  grub_memset (params, 0, GRUB_LINUX_CL_END_OFFSET);
  grub_memcpy (&params->setup_sects, &lh.setup_sects, sizeof (lh) - 0x1F1);

  params->ps_mouse = params->padding10 =  0;

  len = 0x400 - sizeof (lh);
  if (grub_file_read (file, (char *) real_mode_mem + sizeof (lh), len) != len)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

  params->type_of_loader = (LINUX_LOADER_ID_GRUB << 4);

  params->cl_magic = GRUB_LINUX_CL_MAGIC;
  params->cl_offset = 0x1000;
  params->cmd_line_ptr = (unsigned long) real_mode_mem + 0x1000;
  params->ramdisk_image = 0;
  params->ramdisk_size = 0;

  params->heap_end_ptr = GRUB_LINUX_HEAP_END_OFFSET;
  params->loadflags |= GRUB_LINUX_FLAG_CAN_USE_HEAP;

  /* These are not needed to be precise, because Linux uses these values
     only to raise an error when the decompression code cannot find good
     space.  */
  params->ext_mem = ((32 * 0x100000) >> 10);
  params->alt_mem = ((32 * 0x100000) >> 10);

  {
    grub_term_output_t term;
    int found = 0;
    FOR_ACTIVE_TERM_OUTPUTS(term)
      if (grub_strcmp (term->name, "vga_text") == 0
	  || grub_strcmp (term->name, "console") == 0)
	{
	  grub_uint16_t pos = grub_term_getxy (term);
	  params->video_cursor_x = pos >> 8;
	  params->video_cursor_y = pos & 0xff;
	  params->video_width = grub_term_width (term);
	  params->video_height = grub_term_height (term);
	  found = 1;
	  break;
	}
    if (!found)
      {
	params->video_cursor_x = 0;
	params->video_cursor_y = 0;
	params->video_width = 80;
	params->video_height = 25;
      }
  }
  params->video_page = 0; /* ??? */
  params->video_mode = grub_efi_system_table->con_out->mode->mode;
  params->video_ega_bx = 0;
  params->have_vga = 0;
  params->font_size = 16; /* XXX */

  if (grub_le_to_cpu16 (params->version) >= 0x0206)
    {
      params->v0206.efi_signature = GRUB_LINUX_EFI_SIGNATURE;
      params->v0206.efi_system_table = (grub_uint32_t) (unsigned long) grub_efi_system_table;
#ifdef __x86_64__
      params->v0206.efi_system_table_hi = (grub_uint32_t) ((grub_uint64_t) grub_efi_system_table >> 32);
#endif
    }
  else if (grub_le_to_cpu16 (params->version) >= 0x0204)
    {
      params->v0204.efi_signature = GRUB_LINUX_EFI_SIGNATURE_0204;
      params->v0204.efi_system_table = (grub_uint32_t) (unsigned long) grub_efi_system_table;
    }

#if 0
  /* The structure is zeroed already.  */

  /* No VBE on EFI.  */
  params->lfb_width = 0;
  params->lfb_height = 0;
  params->lfb_depth = 0;
  params->lfb_base = 0;
  params->lfb_size = 0;
  params->lfb_line_len = 0;
  params->red_mask_size = 0;
  params->red_field_pos = 0;
  params->green_mask_size = 0;
  params->green_field_pos = 0;
  params->blue_mask_size = 0;
  params->blue_field_pos = 0;
  params->reserved_mask_size = 0;
  params->reserved_field_pos = 0;
  params->vesapm_segment = 0;
  params->vesapm_offset = 0;
  params->lfb_pages = 0;
  params->vesa_attrib = 0;

  /* No APM on EFI.  */
  params->apm_version = 0;
  params->apm_code_segment = 0;
  params->apm_entry = 0;
  params->apm_16bit_code_segment = 0;
  params->apm_data_segment = 0;
  params->apm_flags = 0;
  params->apm_code_len = 0;
  params->apm_data_len = 0;

  /* XXX is there any way to use SpeedStep on EFI?  */
  params->ist_signature = 0;
  params->ist_command = 0;
  params->ist_event = 0;
  params->ist_perf_level = 0;

  /* Let the kernel probe the information.  */
  grub_memset (params->hd0_drive_info, 0, sizeof (params->hd0_drive_info));
  grub_memset (params->hd1_drive_info, 0, sizeof (params->hd1_drive_info));

  /* No MCA on EFI.  */
  params->rom_config_len = 0;

  /* No need to fake the BIOS's memory map.  */
  params->mmap_size = 0;

  /* Let the kernel probe the information.  */
  params->ps_mouse = 0;

  /* Clear padding for future compatibility.  */
  grub_memset (params->padding1, 0, sizeof (params->padding1));
  grub_memset (params->padding2, 0, sizeof (params->padding2));
  grub_memset (params->padding3, 0, sizeof (params->padding3));
  grub_memset (params->padding4, 0, sizeof (params->padding4));
  grub_memset (params->padding5, 0, sizeof (params->padding5));
  grub_memset (params->padding6, 0, sizeof (params->padding6));
  grub_memset (params->padding7, 0, sizeof (params->padding7));
  grub_memset (params->padding8, 0, sizeof (params->padding8));
  grub_memset (params->padding9, 0, sizeof (params->padding9));

#endif

  /* The other EFI parameters are filled when booting.  */

  grub_file_seek (file, real_size + GRUB_DISK_SECTOR_SIZE);

  /* XXX there is no way to know if the kernel really supports EFI.  */
  grub_printf ("   [Linux-bzImage, setup=0x%x, size=0x%x]\n",
	       (unsigned) real_size, (unsigned) prot_size);

  grub_linux_setup_video (params);

  /* Detect explicitly specified memory size, if any.  */
  linux_mem_size = 0;
  for (i = 1; i < argc; i++)
    if (grub_memcmp (argv[i], "mem=", 4) == 0)
      {
	char *val = argv[i] + 4;

	linux_mem_size = grub_strtoul (val, &val, 0);

	if (grub_errno)
	  {
	    grub_errno = GRUB_ERR_NONE;
	    linux_mem_size = 0;
	  }
	else
	  {
	    int shift = 0;

	    switch (grub_tolower (val[0]))
	      {
	      case 'g':
		shift += 10;
                break;
	      case 'm':
		shift += 10;
                break;
	      case 'k':
		shift += 10;
                break;
	      default:
		break;
	      }

	    /* Check an overflow.  */
	    if (linux_mem_size > (~0UL >> shift))
	      linux_mem_size = 0;
	    else
	      linux_mem_size <<= shift;
	  }
      }
    else if (grub_memcmp (argv[i], "video=efifb", 11) == 0)
      {
	if (params->have_vga)
	  params->have_vga = GRUB_VIDEO_LINUX_TYPE_SIMPLE;
      }

  /* Specify the boot file.  */
  dest = grub_stpcpy ((char *) real_mode_mem + GRUB_LINUX_CL_OFFSET,
		      "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);

  /* Copy kernel parameters.  */
  for (i = 1;
       i < argc
	 && dest + grub_strlen (argv[i]) + 1 < ((char *) real_mode_mem
						+ GRUB_LINUX_CL_END_OFFSET);
       i++)
    {
      *dest++ = ' ';
      dest = grub_stpcpy (dest, argv[i]);
    }

  len = prot_size;
  if (grub_file_read (file, (void *) GRUB_LINUX_BZIMAGE_ADDR, len) != len)
    grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");

  if (grub_errno == GRUB_ERR_NONE)
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload, 1);
      loaded = 1;
    }

 fail:

  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_dl_unref (my_mod);
      loaded = 0;
    }

  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_addr_t addr_min, addr_max;
  grub_addr_t addr;
  grub_efi_uintn_t mmap_size;
  grub_efi_memory_descriptor_t *desc;
  grub_efi_uintn_t desc_size;
  struct linux_kernel_header *lh;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      goto fail;
    }

  if (! loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "you need to load the kernel first");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  size = grub_file_size (file);
  initrd_pages = (page_align (size) >> 12);

  lh = (struct linux_kernel_header *) real_mode_mem;

  addr_max = (grub_cpu_to_le32 (lh->initrd_addr_max) << 10);
  if (linux_mem_size != 0 && linux_mem_size < addr_max)
    addr_max = linux_mem_size;

  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  /* Usually, the compression ratio is about 50%.  */
  addr_min = (grub_addr_t) prot_mode_mem + ((prot_mode_pages * 3) << 12)
	     + page_align (size);

  /* Find the highest address to put the initrd.  */
  mmap_size = find_mmap_size ();
  if (grub_efi_get_memory_map (&mmap_size, mmap_buf, 0, &desc_size, 0) <= 0)
    grub_fatal ("cannot get memory map");

  addr = 0;
  for (desc = mmap_buf;
       desc < NEXT_MEMORY_DESCRIPTOR (mmap_buf, mmap_size);
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY
	  && desc->num_pages >= initrd_pages)
	{
	  grub_efi_physical_address_t physical_end;

	  physical_end = desc->physical_start + (desc->num_pages << 12);
	  if (physical_end > addr_max)
	    physical_end = addr_max;

	  if (physical_end < page_align (size))
	    continue;

	  physical_end -= page_align (size);

	  if ((physical_end >= addr_min) &&
	      (physical_end >= desc->physical_start) &&
	      (physical_end > addr))
	    addr = physical_end;
	}
    }

  if (addr == 0)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "no free pages available");
      goto fail;
    }

  initrd_mem = grub_efi_allocate_pages (addr, initrd_pages);
  if (! initrd_mem)
    grub_fatal ("cannot allocate pages");

  if (grub_file_read (file, initrd_mem, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

  grub_printf ("   [Initrd, addr=0x%x, size=0x%x]\n",
	       (unsigned) addr, (unsigned) size);

  lh->ramdisk_image = addr;
  lh->ramdisk_size = size;
  lh->root_dev = 0x0100; /* XXX */

 fail:
  if (file)
    grub_file_close (file);

  return grub_errno;
}

static grub_command_t cmd_linux, cmd_initrd;

GRUB_MOD_INIT(linux)
{
  cmd_linux = grub_register_command ("linux", grub_cmd_linux,
				     0, N_("Load Linux."));
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, N_("Load initrd."));
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
