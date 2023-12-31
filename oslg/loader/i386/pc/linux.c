/* linux.c - boot Linux zImage or bzImage */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/err.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
#include <grub/dl.h>
#include <grub/cpu/linux.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/mm.h>
#include <grub/video.h>

#define GRUB_LINUX_CL_OFFSET		0x9000
#define GRUB_LINUX_CL_END_OFFSET	0x90FF

static grub_dl_t my_mod;

static grub_size_t linux_mem_size;
static int loaded;

static grub_err_t
grub_linux_unload (void)
{
  grub_dl_unref (my_mod);
  loaded = 0;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux16_boot (void)
{
  grub_video_set_mode ("text", 0, 0);
  grub_linux16_real_boot ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  struct linux_kernel_header lh;
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

  if ((grub_size_t) grub_file_size (file) > grub_os_area_size)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "too big kernel (0x%x > 0x%x)",
		  (grub_size_t) grub_file_size (file),
		  grub_os_area_size);
      goto fail;
    }

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

  grub_linux_is_bzimage = 0;
  setup_sects = lh.setup_sects;
  linux_mem_size = 0;

  if (lh.header == grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      && grub_le_to_cpu16 (lh.version) >= 0x0200)
    {
      grub_linux_is_bzimage = (lh.loadflags & GRUB_LINUX_FLAG_BIG_KERNEL);
      lh.type_of_loader = GRUB_LINUX_BOOT_LOADER_TYPE;

      /* Put the real mode part at as a high location as possible.  */
      grub_linux_real_addr
	= (char *) UINT_TO_PTR (grub_mmap_get_lower ()
				- GRUB_LINUX_SETUP_MOVE_SIZE);
      /* But it must not exceed the traditional area.  */
      if (grub_linux_real_addr > (char *) GRUB_LINUX_OLD_REAL_MODE_ADDR)
	grub_linux_real_addr = (char *) GRUB_LINUX_OLD_REAL_MODE_ADDR;

      if (grub_le_to_cpu16 (lh.version) >= 0x0201)
	{
	  lh.heap_end_ptr = grub_cpu_to_le16 (GRUB_LINUX_HEAP_END_OFFSET);
	  lh.loadflags |= GRUB_LINUX_FLAG_CAN_USE_HEAP;
	}

      if (grub_le_to_cpu16 (lh.version) >= 0x0202)
	lh.cmd_line_ptr = grub_linux_real_addr + GRUB_LINUX_CL_OFFSET;
      else
	{
	  lh.cl_magic = grub_cpu_to_le16 (GRUB_LINUX_CL_MAGIC);
	  lh.cl_offset = grub_cpu_to_le16 (GRUB_LINUX_CL_OFFSET);
	  lh.setup_move_size = grub_cpu_to_le16 (GRUB_LINUX_SETUP_MOVE_SIZE);
	}
    }
  else
    {
      /* Your kernel is quite old...  */
      lh.cl_magic = grub_cpu_to_le16 (GRUB_LINUX_CL_MAGIC);
      lh.cl_offset = grub_cpu_to_le16 (GRUB_LINUX_CL_OFFSET);

      setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;

      grub_linux_real_addr = (char *) GRUB_LINUX_OLD_REAL_MODE_ADDR;
    }

  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << GRUB_DISK_SECTOR_BITS;
  prot_size = grub_file_size (file) - real_size - GRUB_DISK_SECTOR_SIZE;

  grub_linux_tmp_addr = (char *) GRUB_LINUX_BZIMAGE_ADDR + prot_size;

  if (! grub_linux_is_bzimage
      && ((char *) GRUB_LINUX_ZIMAGE_ADDR + prot_size > grub_linux_real_addr))
    {
      grub_error (GRUB_ERR_BAD_OS, "too big zImage (0x%x > 0x%x), use bzImage instead",
		  (char *) GRUB_LINUX_ZIMAGE_ADDR + prot_size,
		  (grub_size_t) grub_linux_real_addr);
      goto fail;
    }

  if (grub_linux_real_addr + GRUB_LINUX_SETUP_MOVE_SIZE
      > (char *) UINT_TO_PTR (grub_mmap_get_lower ()))
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE,
		 "too small lower memory (0x%x > 0x%x)",
		  grub_linux_real_addr + GRUB_LINUX_SETUP_MOVE_SIZE,
		  (int) grub_mmap_get_lower ());
      goto fail;
    }

  grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
	       grub_linux_is_bzimage ? "bzImage" : "zImage", real_size, prot_size);

  for (i = 1; i < argc; i++)
    if (grub_memcmp (argv[i], "vga=", 4) == 0)
      {
	/* Video mode selection support.  */
	grub_uint16_t vid_mode;
	char *val = argv[i] + 4;

	if (grub_strcmp (val, "normal") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_NORMAL;
	else if (grub_strcmp (val, "ext") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_EXTENDED;
	else if (grub_strcmp (val, "ask") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_ASK;
	else
	  vid_mode = (grub_uint16_t) grub_strtoul (val, 0, 0);

	if (grub_errno)
	  goto fail;

	lh.vid_mode = grub_cpu_to_le16 (vid_mode);
      }
    else if (grub_memcmp (argv[i], "mem=", 4) == 0)
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

  /* Put the real mode code at the temporary address.  */
  grub_memmove (grub_linux_tmp_addr, &lh, sizeof (lh));

  len = real_size + GRUB_DISK_SECTOR_SIZE - sizeof (lh);
  if (grub_file_read (file, grub_linux_tmp_addr + sizeof (lh), len) != len)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

  if (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      || grub_le_to_cpu16 (lh.version) < 0x0200)
    /* Clear the heap space.  */
    grub_memset (grub_linux_tmp_addr
		 + ((setup_sects + 1) << GRUB_DISK_SECTOR_BITS),
		 0,
		 ((GRUB_LINUX_MAX_SETUP_SECTS - setup_sects - 1)
		  << GRUB_DISK_SECTOR_BITS));

  /* Specify the boot file.  */
  dest = grub_stpcpy (grub_linux_tmp_addr + GRUB_LINUX_CL_OFFSET,
		      "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);

  /* Copy kernel parameters.  */
  for (i = 1;
       i < argc
	 && dest + grub_strlen (argv[i]) + 1 < (grub_linux_tmp_addr
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
      grub_linux_prot_size = prot_size;
      grub_loader_set (grub_linux16_boot, grub_linux_unload, 1);
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
  grub_addr_t addr_max, addr_min, addr;
  struct linux_kernel_header *lh;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "you need to load the kernel first");
      goto fail;
    }

  lh = (struct linux_kernel_header *) grub_linux_tmp_addr;

  if (!(lh->header == grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
	&& grub_le_to_cpu16 (lh->version) >= 0x0200))
    {
      grub_error (GRUB_ERR_BAD_OS, "the kernel is too old for initrd");
      goto fail;
    }

  /* Get the highest address available for the initrd.  */
  if (grub_le_to_cpu16 (lh->version) >= 0x0203)
    {
      addr_max = grub_cpu_to_le32 (lh->initrd_addr_max);

      /* XXX in reality, Linux specifies a bogus value, so
	 it is necessary to make sure that ADDR_MAX does not exceed
	 0x3fffffff.  */
      if (addr_max > GRUB_LINUX_INITRD_MAX_ADDRESS)
	addr_max = GRUB_LINUX_INITRD_MAX_ADDRESS;
    }
  else
    addr_max = GRUB_LINUX_INITRD_MAX_ADDRESS;

  if (linux_mem_size != 0 && linux_mem_size < addr_max)
    addr_max = linux_mem_size;

  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  if (addr_max > grub_os_area_addr + grub_os_area_size)
    addr_max = grub_os_area_addr + grub_os_area_size;

  addr_min = (grub_addr_t) grub_linux_tmp_addr + GRUB_LINUX_CL_END_OFFSET;

  file = grub_file_open (argv[0]);
  if (!file)
    goto fail;

  size = grub_file_size (file);

  /* Put the initrd as high as possible, 4KiB aligned.  */
  addr = (addr_max - size) & ~0xFFF;

  if (addr < addr_min)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "the initrd is too big");
      goto fail;
    }

  if (grub_file_read (file, (void *) addr, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

  lh->ramdisk_image = addr;
  lh->ramdisk_size = size;

 fail:
  if (file)
    grub_file_close (file);

  return grub_errno;
}

static grub_command_t cmd_linux, cmd_initrd;

GRUB_MOD_INIT(linux16)
{
  cmd_linux =
    grub_register_command ("linux16", grub_cmd_linux,
			   0, N_("Load Linux."));
  cmd_initrd =
    grub_register_command ("initrd16", grub_cmd_initrd,
			   0, N_("Load initrd."));
  my_mod = mod;
}

GRUB_MOD_FINI(linux16)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
