/* xnu.c - load xnu kernel. Thanks to Florian Idelberger for all the
   time he spent testing this
 */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/file.h>
#include <grub/xnu.h>
#include <grub/cpu/xnu.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/loader.h>
#include <grub/machoload.h>
#include <grub/macho.h>
#include <grub/cpu/macho.h>
#include <grub/gzio.h>
#include <grub/command.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/env.h>
#include <grub/i18n.h>

struct grub_xnu_devtree_key *grub_xnu_devtree_root = 0;
static int driverspackagenum = 0;
static int driversnum = 0;
int grub_xnu_is_64bit = 0;

void *grub_xnu_heap_start = 0;
grub_size_t grub_xnu_heap_size = 0;

/* Allocate heap by 32MB-blocks. */
#define GRUB_XNU_HEAP_ALLOC_BLOCK 0x2000000

static grub_err_t
grub_xnu_register_memory (char *prefix, int *suffix,
			  void *addr, grub_size_t size);
void *
grub_xnu_heap_malloc (int size)
{
  void *val;
  int oldblknum, newblknum;

  /* The page after the heap is used for stack. Ensure it's usable. */
  if (grub_xnu_heap_size)
    oldblknum = (grub_xnu_heap_size + GRUB_XNU_PAGESIZE
		 + GRUB_XNU_HEAP_ALLOC_BLOCK - 1) / GRUB_XNU_HEAP_ALLOC_BLOCK;
  else
    oldblknum = 0;
  newblknum = (grub_xnu_heap_size + size + GRUB_XNU_PAGESIZE
	       + GRUB_XNU_HEAP_ALLOC_BLOCK - 1) / GRUB_XNU_HEAP_ALLOC_BLOCK;
  if (oldblknum != newblknum)
    {
      /* FIXME: instruct realloc to allocate at 1MB if possible once
	 advanced mm is ready. */
      grub_xnu_heap_start
	= XNU_RELOCATOR (realloc) (grub_xnu_heap_start,
				   newblknum
				   * GRUB_XNU_HEAP_ALLOC_BLOCK);
      if (!grub_xnu_heap_start)
	return NULL;
    }

  val = (grub_uint8_t *) grub_xnu_heap_start + grub_xnu_heap_size;
  grub_xnu_heap_size += size;
  grub_dprintf ("xnu", "val=%p\n", val);
  return val;
}

/* Make sure next block of the heap will be aligned.
   Please notice: aligned are pointers AFTER relocation
   and not the current ones. */
grub_err_t
grub_xnu_align_heap (int align)
{
  int align_overhead = align - grub_xnu_heap_size % align;
  if (align_overhead == align)
    return GRUB_ERR_NONE;
  if (! grub_xnu_heap_malloc (align_overhead))
    return grub_errno;
  return GRUB_ERR_NONE;
}

/* Free subtree pointed by CUR. */
void
grub_xnu_free_devtree (struct grub_xnu_devtree_key *cur)
{
  struct grub_xnu_devtree_key *d;
  while (cur)
    {
      grub_free (cur->name);
      if (cur->datasize == -1)
	grub_xnu_free_devtree (cur->first_child);
      else if (cur->data)
	grub_free (cur->data);
      d = cur->next;
      grub_free (cur);
      cur = d;
    }
}

/* Compute the size of device tree in xnu format. */
static grub_size_t
grub_xnu_writetree_get_size (struct grub_xnu_devtree_key *start, char *name)
{
  grub_size_t ret;
  struct grub_xnu_devtree_key *cur;

  /* Key header. */
  ret = 2 * sizeof (grub_uint32_t);

  /* "name" value. */
  ret += 32 + sizeof (grub_uint32_t)
    + grub_strlen (name) + 4
    - (grub_strlen (name) % 4);

  for (cur = start; cur; cur = cur->next)
    if (cur->datasize != -1)
      {
	int align_overhead;

	align_overhead = 4 - (cur->datasize % 4);
	if (align_overhead == 4)
	  align_overhead = 0;
	ret += 32 + sizeof (grub_uint32_t) + cur->datasize + align_overhead;
      }
    else
      ret += grub_xnu_writetree_get_size (cur->first_child, cur->name);
  return ret;
}

/* Write devtree in XNU format at curptr assuming the head is named NAME.*/
static void *
grub_xnu_writetree_toheap_real (void *curptr,
				struct grub_xnu_devtree_key *start, char *name)
{
  struct grub_xnu_devtree_key *cur;
  int nkeys = 0, nvals = 0;
  for (cur = start; cur; cur = cur->next)
    {
      if (cur->datasize == -1)
	nkeys++;
      else
	nvals++;
    }
  /* For the name. */
  nvals++;

  *((grub_uint32_t *) curptr) = nvals;
  curptr = ((grub_uint32_t *) curptr) + 1;
  *((grub_uint32_t *) curptr) = nkeys;
  curptr = ((grub_uint32_t *) curptr) + 1;

  /* First comes "name" value. */
  grub_memset (curptr, 0, 32);
  grub_memcpy (curptr, "name", 4);
  curptr = ((grub_uint8_t *) curptr) + 32;
  *((grub_uint32_t *)curptr) = grub_strlen (name) + 1;
  curptr = ((grub_uint32_t *) curptr) + 1;
  grub_memcpy (curptr, name, grub_strlen (name));
  curptr = ((grub_uint8_t *) curptr) + grub_strlen (name);
  grub_memset (curptr, 0, 4 - (grub_strlen (name) % 4));
  curptr = ((grub_uint8_t *) curptr) + (4 - (grub_strlen (name) % 4));

  /* Then the other values. */
  for (cur = start; cur; cur = cur->next)
    if (cur->datasize != -1)
      {
	int align_overhead;

	align_overhead = 4 - (cur->datasize % 4);
	if (align_overhead == 4)
	  align_overhead = 0;
	grub_memset (curptr, 0, 32);
	grub_strncpy (curptr, cur->name, 31);
	curptr = ((grub_uint8_t *) curptr) + 32;
	*((grub_uint32_t *) curptr) = cur->datasize;
	curptr = ((grub_uint32_t *) curptr) + 1;
	grub_memcpy (curptr, cur->data, cur->datasize);
	curptr = ((grub_uint8_t *) curptr) + cur->datasize;
	grub_memset (curptr, 0, align_overhead);
	curptr = ((grub_uint8_t *) curptr) + align_overhead;
      }

  /* And then the keys. Recursively use this function. */
  for (cur = start; cur; cur = cur->next)
    if (cur->datasize == -1)
      if (!(curptr = grub_xnu_writetree_toheap_real (curptr,
						     cur->first_child,
						     cur->name)))
	return 0;
  return curptr;
}

grub_err_t
grub_xnu_writetree_toheap (void **start, grub_size_t *size)
{
  struct grub_xnu_devtree_key *chosen;
  struct grub_xnu_devtree_key *memorymap;
  struct grub_xnu_devtree_key *driverkey;
  struct grub_xnu_extdesc *extdesc;
  grub_err_t err;

  err = grub_xnu_align_heap (GRUB_XNU_PAGESIZE);
  if (err)
    return err;

  /* Device tree itself is in the memory map of device tree. */
  /* Create a dummy value in memory-map. */
  chosen = grub_xnu_create_key (&grub_xnu_devtree_root, "chosen");
  if (! chosen)
    return grub_errno;
  memorymap = grub_xnu_create_key (&(chosen->first_child), "memory-map");
  if (! memorymap)
    return grub_errno;

  driverkey = (struct grub_xnu_devtree_key *) grub_malloc (sizeof (*driverkey));
  if (! driverkey)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't write device tree");
  driverkey->name = grub_strdup ("DeviceTree");
  if (! driverkey->name)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't write device tree");
  driverkey->datasize = sizeof (*extdesc);
  driverkey->next = memorymap->first_child;
  memorymap->first_child = driverkey;
  driverkey->data = extdesc
    = (struct grub_xnu_extdesc *) grub_malloc (sizeof (*extdesc));
  if (! driverkey->data)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't write device tree");

  /* Allocate the space based on the size with dummy value. */
  *size = grub_xnu_writetree_get_size (grub_xnu_devtree_root, "/");
  *start = grub_xnu_heap_malloc (*size + GRUB_XNU_PAGESIZE
				 - *size % GRUB_XNU_PAGESIZE);

  /* Put real data in the dummy. */
  extdesc->addr = (grub_uint8_t *) *start - (grub_uint8_t *) grub_xnu_heap_start
    + grub_xnu_heap_will_be_at;
  extdesc->size = (grub_uint32_t) *size;

  /* Write the tree to heap. */
  grub_xnu_writetree_toheap_real (*start, grub_xnu_devtree_root, "/");
  return GRUB_ERR_NONE;
}

/* Find a key or value in parent key. */
struct grub_xnu_devtree_key *
grub_xnu_find_key (struct grub_xnu_devtree_key *parent, char *name)
{
  struct grub_xnu_devtree_key *cur;
  for (cur = parent; cur; cur = cur->next)
    if (grub_strcmp (cur->name, name) == 0)
      return cur;
  return 0;
}

struct grub_xnu_devtree_key *
grub_xnu_create_key (struct grub_xnu_devtree_key **parent, char *name)
{
  struct grub_xnu_devtree_key *ret;
  ret = grub_xnu_find_key (*parent, name);
  if (ret)
    return ret;
  ret = (struct grub_xnu_devtree_key *) grub_zalloc (sizeof (*ret));
  if (! ret)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't create key %s", name);
      return 0;
    }
  ret->name = grub_strdup (name);
  if (! ret->name)
    {
      grub_free (ret);
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't create key %s", name);
      return 0;
    }
  ret->datasize = -1;
  ret->next = *parent;
  *parent = ret;
  return ret;
}

struct grub_xnu_devtree_key *
grub_xnu_create_value (struct grub_xnu_devtree_key **parent, char *name)
{
  struct grub_xnu_devtree_key *ret;
  ret = grub_xnu_find_key (*parent, name);
  if (ret)
    {
      if (ret->datasize == -1)
	grub_xnu_free_devtree (ret->first_child);
      else if (ret->datasize)
	grub_free (ret->data);
      ret->datasize = 0;
      ret->data = 0;
      return ret;
    }
  ret = (struct grub_xnu_devtree_key *) grub_zalloc (sizeof (*ret));
  if (! ret)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't create value %s", name);
      return 0;
    }
  ret->name = grub_strdup (name);
  if (! ret->name)
    {
      grub_free (ret);
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't create value %s", name);
      return 0;
    }
  ret->next = *parent;
  *parent = ret;
  return ret;
}

static grub_err_t
grub_xnu_unload (void)
{
  grub_cpu_xnu_unload ();

  grub_xnu_free_devtree (grub_xnu_devtree_root);
  grub_xnu_devtree_root = 0;

  /* Free loaded image. */
  driversnum = 0;
  driverspackagenum = 0;
  grub_free (grub_xnu_heap_start);
  grub_xnu_heap_start = 0;
  grub_xnu_heap_size = 0;
  grub_xnu_unlock ();
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_xnu_kernel (grub_command_t cmd __attribute__ ((unused)),
		     int argc, char *args[])
{
  grub_err_t err;
  grub_macho_t macho;
  grub_uint32_t startcode, endcode;
  int i;
  char *ptr, *loadaddr;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_xnu_unload ();

  macho = grub_macho_open (args[0]);
  if (! macho)
    return grub_errno;
  if (! grub_macho_contains_macho32 (macho))
    {
      grub_macho_close (macho);
      return grub_error (GRUB_ERR_BAD_OS,
			 "kernel doesn't contain suitable 32-bit architecture");
    }

  err = grub_macho_size32 (macho, &startcode, &endcode, GRUB_MACHO_NOBSS);
  if (err)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return err;
    }

  grub_dprintf ("xnu", "endcode = %lx, startcode = %lx\n",
		(unsigned long) endcode, (unsigned long) startcode);

  loadaddr = grub_xnu_heap_malloc (endcode - startcode);
  grub_xnu_heap_will_be_at = startcode;

  if (! loadaddr)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			 "not enough memory to load kernel");
    }

  /* Load kernel. */
  err = grub_macho_load32 (macho, loadaddr - startcode, GRUB_MACHO_NOBSS);
  if (err)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return err;
    }

  grub_xnu_entry_point = grub_macho_get_entry_point32 (macho);
  if (! grub_xnu_entry_point)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return grub_error (GRUB_ERR_BAD_OS, "couldn't find entry point");
    }

  grub_macho_close (macho);

  err = grub_xnu_align_heap (GRUB_XNU_PAGESIZE);
  if (err)
    {
      grub_xnu_unload ();
      return err;
    }

  /* Copy parameters to kernel command line. */
  ptr = grub_xnu_cmdline;
  for (i = 1; i < argc; i++)
    {
      if (ptr + grub_strlen (args[i]) + 1
	  >= grub_xnu_cmdline + sizeof (grub_xnu_cmdline))
	break;
      grub_memcpy (ptr, args[i], grub_strlen (args[i]));
      ptr += grub_strlen (args[i]);
      *ptr = ' ';
      ptr++;
    }

  /* Replace last space by '\0'. */
  if (ptr != grub_xnu_cmdline)
    *(ptr - 1) = 0;

  grub_loader_set (grub_xnu_boot, grub_xnu_unload, 0);

  grub_xnu_lock ();
  grub_xnu_is_64bit = 0;

  return 0;
}

static grub_err_t
grub_cmd_xnu_kernel64 (grub_command_t cmd __attribute__ ((unused)),
		       int argc, char *args[])
{
  grub_err_t err;
  grub_macho_t macho;
  grub_uint64_t startcode, endcode;
  int i;
  char *ptr, *loadaddr;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_xnu_unload ();

  macho = grub_macho_open (args[0]);
  if (! macho)
    return grub_errno;
  if (! grub_macho_contains_macho64 (macho))
    {
      grub_macho_close (macho);
      return grub_error (GRUB_ERR_BAD_OS,
			 "kernel doesn't contain suitable 64-bit architecture");
    }

  err = grub_macho_size64 (macho, &startcode, &endcode, GRUB_MACHO_NOBSS);
  if (err)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return err;
    }

  startcode &= 0x0fffffff;
  endcode &= 0x0fffffff;

  grub_dprintf ("xnu", "endcode = %lx, startcode = %lx\n",
		(unsigned long) endcode, (unsigned long) startcode);

  loadaddr = grub_xnu_heap_malloc (endcode - startcode);
  grub_xnu_heap_will_be_at = startcode;

  if (! loadaddr)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			 "not enough memory to load kernel");
    }

  /* Load kernel. */
  err = grub_macho_load64 (macho, loadaddr - startcode, GRUB_MACHO_NOBSS);
  if (err)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return err;
    }

  grub_xnu_entry_point = grub_macho_get_entry_point64 (macho) & 0x0fffffff;
  if (! grub_xnu_entry_point)
    {
      grub_macho_close (macho);
      grub_xnu_unload ();
      return grub_error (GRUB_ERR_BAD_OS, "couldn't find entry point");
    }

  grub_macho_close (macho);

  err = grub_xnu_align_heap (GRUB_XNU_PAGESIZE);
  if (err)
    {
      grub_xnu_unload ();
      return err;
    }

  /* Copy parameters to kernel command line. */
  ptr = grub_xnu_cmdline;
  for (i = 1; i < argc; i++)
    {
      if (ptr + grub_strlen (args[i]) + 1
	  >= grub_xnu_cmdline + sizeof (grub_xnu_cmdline))
	break;
      grub_memcpy (ptr, args[i], grub_strlen (args[i]));
      ptr += grub_strlen (args[i]);
      *ptr = ' ';
      ptr++;
    }

  /* Replace last space by '\0'. */
  if (ptr != grub_xnu_cmdline)
    *(ptr - 1) = 0;

  grub_loader_set (grub_xnu_boot, grub_xnu_unload, 0);

  grub_xnu_lock ();
  grub_xnu_is_64bit = 1;

  return 0;
}

/* Register a memory in a memory map under name PREFIXSUFFIX
   and increment SUFFIX. */
static grub_err_t
grub_xnu_register_memory (char *prefix, int *suffix,
			  void *addr, grub_size_t size)
{
  struct grub_xnu_devtree_key *chosen;
  struct grub_xnu_devtree_key *memorymap;
  struct grub_xnu_devtree_key *driverkey;
  struct grub_xnu_extdesc *extdesc;

  if (! grub_xnu_heap_size)
    return grub_error (GRUB_ERR_BAD_OS, "no xnu kernel loaded");

  chosen = grub_xnu_create_key (&grub_xnu_devtree_root, "chosen");
  if (! chosen)
    return grub_errno;
  memorymap = grub_xnu_create_key (&(chosen->first_child), "memory-map");
  if (! memorymap)
    return grub_errno;

  driverkey = (struct grub_xnu_devtree_key *) grub_malloc (sizeof (*driverkey));
  if (! driverkey)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't register memory");
  if (suffix)
    {
      driverkey->name = grub_xasprintf ("%s%d", prefix, (*suffix)++);
      if (!driverkey->name)
	return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't register memory");
    }
  else
    driverkey->name = grub_strdup (prefix);
  if (! driverkey->name)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't register extension");
  driverkey->datasize = sizeof (*extdesc);
  driverkey->next = memorymap->first_child;
  memorymap->first_child = driverkey;
  driverkey->data = extdesc
    = (struct grub_xnu_extdesc *) grub_malloc (sizeof (*extdesc));
  if (! driverkey->data)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "can't register extension");
  extdesc->addr = grub_xnu_heap_will_be_at +
    ((grub_uint8_t *) addr - (grub_uint8_t *) grub_xnu_heap_start);
  extdesc->size = (grub_uint32_t) size;
  return GRUB_ERR_NONE;
}

static inline char *
get_name_ptr (char *name)
{
  char *p = name, *p2;
  /* Skip Info.plist.  */
  p2 = grub_strrchr (p, '/');
  if (!p2)
    return name;
  if (p2 == name)
    return name + 1;
  p = p2 - 1;

  p2 = grub_strrchr (p, '/');
  if (!p2)
    return name;
  if (p2 == name)
    return name + 1;
  if (grub_memcmp (p2, "/Contents/", sizeof ("/Contents/") - 1) != 0)
    return p2 + 1;

  p = p2 - 1;

  p2 = grub_strrchr (p, '/');
  if (!p2)
    return name;
  return p2 + 1;
}

/* Load .kext. */
static grub_err_t
grub_xnu_load_driver (char *infoplistname, grub_file_t binaryfile)
{
  grub_macho_t macho;
  grub_err_t err;
  grub_file_t infoplist;
  struct grub_xnu_extheader *exthead;
  int neededspace = sizeof (*exthead);
  grub_uint8_t *buf;
  grub_size_t infoplistsize = 0, machosize = 0;
  char *name, *nameend;
  int namelen;

  name = get_name_ptr (infoplistname);
  nameend = grub_strchr (name, '/');

  if (nameend)
    namelen = nameend - name;
  else
    namelen = grub_strlen (name);

  neededspace += namelen + 1;

  if (! grub_xnu_heap_size)
    return grub_error (GRUB_ERR_BAD_OS, "no xnu kernel loaded");

  /* Compute the needed space. */
  if (binaryfile)
    {
      macho = grub_macho_file (binaryfile);
      if (! macho || ! grub_macho_contains_macho32 (macho))
	{
	  if (macho)
	    grub_macho_close (macho);
	  return grub_error (GRUB_ERR_BAD_OS,
			     "extension doesn't contain suitable architecture");
	}
      if (grub_xnu_is_64bit)
	machosize = grub_macho_filesize64 (macho);
      else
	machosize = grub_macho_filesize32 (macho);
      neededspace += machosize;
    }
  else
    macho = 0;

  if (infoplistname)
    infoplist = grub_gzfile_open (infoplistname, 1);
  else
    infoplist = 0;
  grub_errno = GRUB_ERR_NONE;
  if (infoplist)
    {
      infoplistsize = grub_file_size (infoplist);
      neededspace += infoplistsize + 1;
    }
  else
    infoplistsize = 0;

  /* Allocate the space. */
  err = grub_xnu_align_heap (GRUB_XNU_PAGESIZE);
  if (err)
    return err;
  buf = grub_xnu_heap_malloc (neededspace);

  exthead = (struct grub_xnu_extheader *) buf;
  grub_memset (exthead, 0, sizeof (*exthead));
  buf += sizeof (*exthead);

  /* Load the binary. */
  if (macho)
    {
      exthead->binaryaddr = (buf - (grub_uint8_t *) grub_xnu_heap_start)
	+ grub_xnu_heap_will_be_at;
      exthead->binarysize = machosize;
      if (grub_xnu_is_64bit)
	err = grub_macho_readfile64 (macho, buf);
      else
	err = grub_macho_readfile32 (macho, buf);
      if (err)
	{
	  grub_macho_close (macho);
	  return err;
	}
      grub_macho_close (macho);
      buf += machosize;
    }
  grub_errno = GRUB_ERR_NONE;

  /* Load the plist. */
  if (infoplist)
    {
      exthead->infoplistaddr = (buf - (grub_uint8_t *) grub_xnu_heap_start)
	+ grub_xnu_heap_will_be_at;
      exthead->infoplistsize = infoplistsize + 1;
      if (grub_file_read (infoplist, buf, infoplistsize)
	  != (grub_ssize_t) (infoplistsize))
	{
	  grub_file_close (infoplist);
	  grub_error_push ();
	  return grub_error (GRUB_ERR_BAD_OS, "couldn't read file %s: ",
			     infoplistname);
	}
      grub_file_close (infoplist);
      buf[infoplistsize] = 0;
      buf += infoplistsize + 1;
    }
  grub_errno = GRUB_ERR_NONE;

  exthead->nameaddr = (buf - (grub_uint8_t *) grub_xnu_heap_start)
    + grub_xnu_heap_will_be_at;
  exthead->namesize = namelen + 1;
  grub_memcpy (buf, name, namelen);
  buf[namelen] = 0;
  buf += namelen + 1;

  /* Announce to kernel */
  return grub_xnu_register_memory ("Driver-", &driversnum, exthead,
				   neededspace);
}

/* Load mkext. */
static grub_err_t
grub_cmd_xnu_mkext (grub_command_t cmd __attribute__ ((unused)),
		    int argc, char *args[])
{
  grub_file_t file;
  void *loadto;
  grub_err_t err;
  grub_off_t readoff = 0;
  grub_ssize_t readlen = -1;
  struct grub_macho_fat_header head;
  struct grub_macho_fat_arch *archs;
  int narchs, i;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  if (! grub_xnu_heap_size)
    return grub_error (GRUB_ERR_BAD_OS, "no xnu kernel loaded");

  file = grub_gzfile_open (args[0], 1);
  if (! file)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND,
		       "couldn't load driver package");

  /* Sometimes caches are fat binary. Errgh. */
  if (grub_file_read (file, &head, sizeof (head))
      != (grub_ssize_t) (sizeof (head)))
    {
      /* I don't know the internal structure of package but
	 can hardly imagine a valid package shorter than 20 bytes. */
      grub_file_close (file);
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS, "couldn't read file %s", args[0]);
    }

  /* Find the corresponding architecture. */
  if (grub_be_to_cpu32 (head.magic) == GRUB_MACHO_FAT_MAGIC)
    {
      narchs = grub_be_to_cpu32 (head.nfat_arch);
      archs = grub_malloc (sizeof (struct grub_macho_fat_arch) * narchs);
      if (! archs)
	{
	  grub_file_close (file);
	  grub_error_push ();
	  return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			     "couldn't read file %s", args[0]);

	}
      if (grub_file_read (file, archs,
			  sizeof (struct grub_macho_fat_arch) * narchs)
	  != (grub_ssize_t) sizeof(struct grub_macho_fat_arch) * narchs)
	{
	  grub_free (archs);
	  grub_error_push ();
	  return grub_error (GRUB_ERR_READ_ERROR, "cannot read fat header");
	}
      for (i = 0; i < narchs; i++)
	{
	  if (!grub_xnu_is_64bit && GRUB_MACHO_CPUTYPE_IS_HOST32
	      (grub_be_to_cpu32 (archs[i].cputype)))
	    {
	      readoff = grub_be_to_cpu32 (archs[i].offset);
	      readlen = grub_be_to_cpu32 (archs[i].size);
	    }
	  if (grub_xnu_is_64bit && GRUB_MACHO_CPUTYPE_IS_HOST64
	      (grub_be_to_cpu32 (archs[i].cputype)))
	    {
	      readoff = grub_be_to_cpu32 (archs[i].offset);
	      readlen = grub_be_to_cpu32 (archs[i].size);
	    }
	}
      grub_free (archs);
    }
  else
    {
      /* It's a flat file. Some sane people still exist. */
      readoff = 0;
      readlen = grub_file_size (file);
    }

  if (readlen == -1)
    {
      grub_file_close (file);
      return grub_error (GRUB_ERR_BAD_OS, "no suitable architecture is found");
    }

  /* Allocate space. */
  err = grub_xnu_align_heap (GRUB_XNU_PAGESIZE);
  if (err)
    {
      grub_file_close (file);
      return err;
    }

  loadto = grub_xnu_heap_malloc (readlen);
  if (! loadto)
    {
      grub_file_close (file);
      return grub_errno;
    }

  /* Read the file. */
  grub_file_seek (file, readoff);
  if (grub_file_read (file, loadto, readlen) != (grub_ssize_t) (readlen))
    {
      grub_file_close (file);
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS, "couldn't read file %s", args[0]);
    }
  grub_file_close (file);

  /* Pass it to kernel. */
  return grub_xnu_register_memory ("DriversPackage-", &driverspackagenum,
				   loadto, readlen);
}

static grub_err_t
grub_cmd_xnu_ramdisk (grub_command_t cmd __attribute__ ((unused)),
		      int argc, char *args[])
{
  grub_file_t file;
  void *loadto;
  grub_err_t err;
  grub_size_t size;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  if (! grub_xnu_heap_size)
    return grub_error (GRUB_ERR_BAD_OS, "no xnu kernel loaded");

  file = grub_gzfile_open (args[0], 1);
  if (! file)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND,
		       "couldn't load ramdisk");

  err = grub_xnu_align_heap (GRUB_XNU_PAGESIZE);
  if (err)
    return err;

  size = grub_file_size (file);

  loadto = grub_xnu_heap_malloc (size);
  if (! loadto)
    return grub_errno;
  if (grub_file_read (file, loadto, size)
      != (grub_ssize_t) (size))
    {
      grub_file_close (file);
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS, "couldn't read file %s", args[0]);
    }
  return grub_xnu_register_memory ("RAMDisk", 0, loadto, size);
}

/* Returns true if the kext should be loaded according to plist
   and osbundlereq. Also fill BINNAME. */
static int
grub_xnu_check_os_bundle_required (char *plistname, char *osbundlereq,
				   char **binname)
{
  grub_file_t file;
  char *buf = 0, *tagstart = 0, *ptr1 = 0, *keyptr = 0;
  char *stringptr = 0, *ptr2 = 0;
  grub_size_t size;
  int depth = 0;
  int ret;
  int osbundlekeyfound = 0, binnamekeyfound = 0;
  if (binname)
    *binname = 0;

  file = grub_gzfile_open (plistname, 1);
  if (! file)
    {
      grub_file_close (file);
      grub_error_push ();
      grub_error (GRUB_ERR_BAD_OS, "couldn't read file %s", plistname);
      return 0;
    }

  size = grub_file_size (file);
  buf = grub_malloc (size);
  if (! buf)
    {
      grub_file_close (file);
      grub_error_push ();
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "couldn't read file %s", plistname);
      return 0;
    }
  if (grub_file_read (file, buf, size) != (grub_ssize_t) (size))
    {
      grub_file_close (file);
      grub_error_push ();
      grub_error (GRUB_ERR_BAD_OS, "couldn't read file %s", plistname);
      return 0;
    }
  grub_file_close (file);

  /* Set the return value for the case when no OSBundleRequired tag is found. */
  if (osbundlereq)
    ret = grub_strword (osbundlereq, "all") || grub_strword (osbundlereq, "-");
  else
    ret = 1;

  /* Parse plist. It's quite dirty and inextensible but does its job. */
  for (ptr1 = buf; ptr1 < buf + size; ptr1++)
    switch (*ptr1)
      {
      case '<':
	tagstart = ptr1;
	*ptr1 = 0;
	if (keyptr && depth == 4
	    && grub_strcmp (keyptr, "OSBundleRequired") == 0)
	  osbundlekeyfound = 1;
	if (keyptr && depth == 4 &&
	    grub_strcmp (keyptr, "CFBundleExecutable") == 0)
	  binnamekeyfound = 1;
	if (stringptr && osbundlekeyfound && osbundlereq && depth == 4)
	  {
	    for (ptr2 = stringptr; *ptr2; ptr2++)
	      *ptr2 = grub_tolower (*ptr2);
	    ret = grub_strword (osbundlereq, stringptr)
	      || grub_strword (osbundlereq, "all");
	  }
	if (stringptr && binnamekeyfound && binname && depth == 4)
	  {
	    if (*binname)
	      grub_free (*binname);
	    *binname = grub_strdup (stringptr);
	  }

	*ptr1 = '<';
	keyptr = 0;
	stringptr = 0;
	break;
      case '>':
	if (! tagstart)
	  {
	    grub_free (buf);
	    grub_error (GRUB_ERR_BAD_OS, "can't parse %s", plistname);
	    return 0;
	  }
	*ptr1 = 0;
	if (tagstart[1] == '?' || ptr1[-1] == '/')
	  {
	    osbundlekeyfound = 0;
	    *ptr1 = '>';
	    break;
	  }
	if (depth == 3 && grub_strcmp (tagstart + 1, "key") == 0)
	  keyptr = ptr1 + 1;
	if (depth == 3 && grub_strcmp (tagstart + 1, "string") == 0)
	  stringptr = ptr1 + 1;
	else if (grub_strcmp (tagstart + 1, "/key") != 0)
	  {
	    osbundlekeyfound = 0;
	    binnamekeyfound = 0;
	  }
	*ptr1 = '>';

	if (tagstart[1] == '/')
	  depth--;
	else
	  depth++;
	break;
      }
  grub_free (buf);

  return ret;
}

struct grub_xnu_scan_dir_for_kexts_closure
{
  char *dirname;
  char *osbundlerequired;
  int maxrecursion;
};

static int
grub_xnu_scan_dir_for_kexts_hook (const char *filename,
				  const struct grub_dirhook_info *info,
				  void *closure)
{
  struct grub_xnu_scan_dir_for_kexts_closure *c = closure;
  char *newdirname;
  if (! info->dir)
    return 0;
  if (filename[0] == '.')
    return 0;

  if (grub_strlen (filename) < 5 ||
      grub_memcmp (filename + grub_strlen (filename) - 5, ".kext", 5) != 0)
    return 0;

  newdirname
    = grub_malloc (grub_strlen (c->dirname) + grub_strlen (filename) + 2);

  /* It's a .kext. Try to load it. */
  if (newdirname)
    {
      grub_strcpy (newdirname, c->dirname);
      newdirname[grub_strlen (newdirname) + 1] = 0;
      newdirname[grub_strlen (newdirname)] = '/';
      grub_strcpy (newdirname + grub_strlen (newdirname), filename);
      grub_xnu_load_kext_from_dir (newdirname, c->osbundlerequired,
				   c->maxrecursion);
      if (grub_errno == GRUB_ERR_BAD_OS)
	grub_errno = GRUB_ERR_NONE;
      grub_free (newdirname);
    }
  return 0;
}

/* Load all loadable kexts placed under DIRNAME and matching OSBUNDLEREQUIRED */
grub_err_t
grub_xnu_scan_dir_for_kexts (char *dirname, char *osbundlerequired,
			     int maxrecursion)
{
  grub_device_t dev;
  char *device_name;
  grub_fs_t fs;
  const char *path;

  if (! grub_xnu_heap_size)
    return grub_error (GRUB_ERR_BAD_OS, "no xnu kernel loaded");

  device_name = grub_file_get_device_name (dirname);
  dev = grub_device_open (device_name);
  if (dev)
    {
      struct grub_xnu_scan_dir_for_kexts_closure c BURG_VAR_UNUSED;

      fs = grub_fs_probe (dev);
      path = grub_strchr (dirname, ')');
      if (! path)
	path = dirname;
      else
	path++;

      c.dirname = dirname;
      c.osbundlerequired = osbundlerequired;
      c.maxrecursion = maxrecursion;
      if (fs)
	(fs->dir) (dev, path, grub_xnu_scan_dir_for_kexts_hook, 0);
      grub_device_close (dev);
    }
  grub_free (device_name);

  return GRUB_ERR_NONE;
}

struct grub_xnu_load_kext_from_dir_closure
{
  char *dirname;
  char *osbundlerequired;
  int maxrecursion;
  int usemacos;
  char *plistname;
  char *newdirname;
};

static int
grub_xnu_load_kext_from_dir_hook (const char *filename,
				  const struct grub_dirhook_info *info,
				  void *closure)
{
  struct grub_xnu_load_kext_from_dir_closure *c = closure;

  if (grub_strlen (filename) > 15)
    return 0;
  grub_strcpy (c->newdirname + grub_strlen (c->dirname) + 1, filename);

  /* If the kext contains directory "Contents" all real stuff is in
     this directory. */
  if (info->dir && grub_strcasecmp (filename, "Contents") == 0)
    grub_xnu_load_kext_from_dir (c->newdirname, c->osbundlerequired,
				 c->maxrecursion - 1);

  /* Directory "Plugins" contains nested kexts. */
  if (info->dir && grub_strcasecmp (filename, "Plugins") == 0)
    grub_xnu_scan_dir_for_kexts (c->newdirname, c->osbundlerequired,
				 c->maxrecursion - 1);

  /* Directory "MacOS" contains executable, otherwise executable is
     on the top. */
  if (info->dir && grub_strcasecmp (filename, "MacOS") == 0)
    c->usemacos = 1;

  /* Info.plist is the file which governs our future actions. */
  if (! info->dir && grub_strcasecmp (filename, "Info.plist") == 0
      && ! c->plistname)
    c->plistname = grub_strdup (c->newdirname);
  return 0;
}

/* Load extension DIRNAME. (extensions are directories in xnu) */
grub_err_t
grub_xnu_load_kext_from_dir (char *dirname, char *osbundlerequired,
			     int maxrecursion)
{
  grub_device_t dev;
  char *newdirname;
  char *newpath;
  char *device_name;
  grub_fs_t fs;
  const char *path;
  char *binsuffix;
  grub_file_t binfile;

  newdirname = grub_malloc (grub_strlen (dirname) + 20);
  if (! newdirname)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "couldn't allocate buffer");
  grub_strcpy (newdirname, dirname);
  newdirname[grub_strlen (dirname)] = '/';
  newdirname[grub_strlen (dirname) + 1] = 0;
  device_name = grub_file_get_device_name (dirname);
  dev = grub_device_open (device_name);
  if (dev)
    {
      struct grub_xnu_load_kext_from_dir_closure c;

      fs = grub_fs_probe (dev);
      path = grub_strchr (dirname, ')');
      if (! path)
	path = dirname;
      else
	path++;

      newpath = grub_strchr (newdirname, ')');
      if (! newpath)
	newpath = newdirname;
      else
	newpath++;

      c.dirname = dirname;
      c.osbundlerequired = osbundlerequired;
      c.maxrecursion = maxrecursion;
      c.usemacos = 0;
      c.plistname = 0;
      c.newdirname = newdirname;
      /* Look at the directory. */
      if (fs)
	(fs->dir) (dev, path, grub_xnu_load_kext_from_dir_hook, &c);

      if (c.plistname && grub_xnu_check_os_bundle_required
	  (c.plistname, osbundlerequired, &binsuffix))
	{
	  if (binsuffix)
	    {
	      /* Open the binary. */
	      char *binname = grub_malloc (grub_strlen (dirname)
					   + grub_strlen (binsuffix)
					   + sizeof ("/MacOS/"));
	      grub_strcpy (binname, dirname);
	      if (c.usemacos)
		grub_strcpy (binname + grub_strlen (binname), "/MacOS/");
	      else
		grub_strcpy (binname + grub_strlen (binname), "/");
	      grub_strcpy (binname + grub_strlen (binname), binsuffix);
	      grub_dprintf ("xnu", "%s:%s\n", c.plistname, binname);
	      binfile = grub_gzfile_open (binname, 1);
	      if (! binfile)
		grub_errno = GRUB_ERR_NONE;

	      /* Load the extension. */
	      grub_xnu_load_driver (c.plistname, binfile);
	      grub_free (binname);
	      grub_free (binsuffix);
	    }
	  else
	    {
	      grub_dprintf ("xnu", "%s:0\n", c.plistname);
	      grub_xnu_load_driver (c.plistname, 0);
	    }
	}
      grub_free (c.plistname);
      grub_device_close (dev);
    }
  grub_free (device_name);

  return GRUB_ERR_NONE;
}


static int locked=0;
static grub_dl_t my_mod;

/* Load the kext. */
static grub_err_t
grub_cmd_xnu_kext (grub_command_t cmd __attribute__ ((unused)),
		   int argc, char *args[])
{
  grub_file_t binfile = 0;
  if (argc == 2)
    {
      /* User explicitly specified plist and binary. */
      if (grub_strcmp (args[1], "-") != 0)
	{
	  binfile = grub_gzfile_open (args[1], 1);
	  if (! binfile)
	    {
	      grub_error (GRUB_ERR_BAD_OS, "can't open file");
	      return GRUB_ERR_NONE;
	    }
	}
      return grub_xnu_load_driver (grub_strcmp (args[0], "-") ? args[0] : 0,
				   binfile);
    }

  /* load kext normally. */
  if (argc == 1)
    return grub_xnu_load_kext_from_dir (args[0], 0, 10);

  return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");
}

/* Load a directory containing kexts. */
static grub_err_t
grub_cmd_xnu_kextdir (grub_command_t cmd __attribute__ ((unused)),
		      int argc, char *args[])
{
  if (argc != 1 && argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "directory name required");

  if (argc == 1)
    return grub_xnu_scan_dir_for_kexts (args[0],
					"console,root,local-root,network-root",
					10);
  else
    {
      char *osbundlerequired = grub_strdup (args[1]), *ptr;
      grub_err_t err;
      if (! osbundlerequired)
	return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			   "couldn't allocate string temporary space");
      for (ptr = osbundlerequired; *ptr; ptr++)
	*ptr = grub_tolower (*ptr);
      err = grub_xnu_scan_dir_for_kexts (args[0], osbundlerequired, 10);
      grub_free (osbundlerequired);
      return err;
    }
}

static inline int
hextoval (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  return 0;
}

static inline void
unescape (char *name, char *curdot, char *nextdot, int *len)
{
  char *ptr, *dptr;
  dptr = name;
  for (ptr = curdot; ptr < nextdot;)
    if (ptr + 2 < nextdot && *ptr == '%')
      {
	*dptr = (hextoval (ptr[1]) << 4) | (hextoval (ptr[2]));
	ptr += 3;
	dptr++;
      }
    else
      {
	*dptr = *ptr;
	ptr++;
	dptr++;
      }
  *len = dptr - name;
}

static int
iterate_env (struct grub_env_var *var, void *closure __attribute__ ((unused)))
{
  char *nextdot = 0, *curdot;
  struct grub_xnu_devtree_key **curkey = &grub_xnu_devtree_root;
  struct grub_xnu_devtree_key *curvalue;
  char *name = 0, *data;
  int len;

  if (grub_memcmp (var->name, "XNU.DeviceTree.",
		   sizeof ("XNU.DeviceTree.") - 1) != 0)
    return 0;

  curdot = var->name + sizeof ("XNU.DeviceTree.") - 1;
  nextdot = grub_strchr (curdot, '.');
  if (nextdot)
    nextdot++;
  while (nextdot)
    {
      name = grub_realloc (name, nextdot - curdot + 1);

      if (!name)
	return 1;

      unescape (name, curdot, nextdot, &len);
      name[len - 1] = 0;

      curkey = &(grub_xnu_create_key (curkey, name)->first_child);

      curdot = nextdot;
      nextdot = grub_strchr (nextdot, '.');
      if (nextdot)
	nextdot++;
    }

  nextdot = curdot + grub_strlen (curdot) + 1;

  name = grub_realloc (name, nextdot - curdot + 1);

  if (!name)
    return 1;

  unescape (name, curdot, nextdot, &len);
  name[len] = 0;

  curvalue = grub_xnu_create_value (curkey, name);
  grub_free (name);

  data = grub_malloc (grub_strlen (var->value) + 1);
  if (!data)
    return 1;

  unescape (data, var->value, var->value + grub_strlen (var->value),
	    &len);
  curvalue->datasize = len;
  curvalue->data = data;

  return 0;
}

grub_err_t
grub_xnu_fill_devicetree (void)
{
  grub_env_iterate (iterate_env, 0);

  return grub_errno;
}

struct grub_video_bitmap *grub_xnu_bitmap = 0;
grub_xnu_bitmap_mode_t grub_xnu_bitmap_mode;

/* Option array indices.  */
#define XNU_SPLASH_CMD_ARGINDEX_MODE 0

static const struct grub_arg_option xnu_splash_cmd_options[] =
  {
    {"mode", 'm', 0, "Background image mode.", "stretch|normal",
     ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_xnu_splash (grub_extcmd_t cmd,
		     int argc, char *args[])
{
  grub_err_t err;
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  if (cmd->state[XNU_SPLASH_CMD_ARGINDEX_MODE].set &&
      grub_strcmp (cmd->state[XNU_SPLASH_CMD_ARGINDEX_MODE].arg,
		   "stretch") == 0)
    grub_xnu_bitmap_mode = GRUB_XNU_BITMAP_STRETCH;
  else
    grub_xnu_bitmap_mode = GRUB_XNU_BITMAP_CENTER;

  err = grub_video_bitmap_load (&grub_xnu_bitmap, args[0]);
  if (err)
    grub_xnu_bitmap = 0;

  return err;
}


#ifndef GRUB_UTIL
static grub_err_t
grub_cmd_xnu_resume (grub_command_t cmd __attribute__ ((unused)),
		     int argc, char *args[])
{
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  return grub_xnu_resume (args[0]);
}
#endif

void
grub_xnu_lock ()
{
  if (!locked)
    grub_dl_ref (my_mod);
  locked = 1;
}

void
grub_xnu_unlock ()
{
  if (locked)
    grub_dl_unref (my_mod);
  locked = 0;
}

static grub_command_t cmd_kernel64, cmd_kernel, cmd_mkext, cmd_kext;
static grub_command_t cmd_kextdir, cmd_ramdisk, cmd_resume;
static grub_extcmd_t cmd_splash;

GRUB_MOD_INIT(xnu)
{
  cmd_kernel = grub_register_command ("xnu_kernel", grub_cmd_xnu_kernel, 0,
				      N_("Load XNU image."));
  cmd_kernel64 = grub_register_command ("xnu_kernel64", grub_cmd_xnu_kernel64,
					0, N_("Load 64-bit XNU image."));
  cmd_mkext = grub_register_command ("xnu_mkext", grub_cmd_xnu_mkext, 0,
				     N_("Load XNU extension package."));
  cmd_kext = grub_register_command ("xnu_kext", grub_cmd_xnu_kext, 0,
				    N_("Load XNU extension."));
  cmd_kextdir = grub_register_command ("xnu_kextdir", grub_cmd_xnu_kextdir,
				       N_("DIRECTORY [OSBundleRequired]"),
				       N_("Load XNU extension directory."));
  cmd_ramdisk = grub_register_command ("xnu_ramdisk", grub_cmd_xnu_ramdisk, 0,
				       "Load XNU ramdisk. "
				       "It will be seen as md0.");
  cmd_splash = grub_register_extcmd ("xnu_splash",
				     grub_cmd_xnu_splash,
				     GRUB_COMMAND_FLAG_BOTH, 0,
				     N_("Load a splash image for XNU."),
				     xnu_splash_cmd_options);

#ifndef GRUB_UTIL
  cmd_resume = grub_register_command ("xnu_resume", grub_cmd_xnu_resume,
				      0, N_("Load XNU hibernate image."));
#endif

  grub_cpu_xnu_init ();

  my_mod = mod;
}

GRUB_MOD_FINI(xnu)
{
#ifndef GRUB_UTIL
  grub_unregister_command (cmd_resume);
#endif
  grub_unregister_command (cmd_mkext);
  grub_unregister_command (cmd_kext);
  grub_unregister_command (cmd_kextdir);
  grub_unregister_command (cmd_ramdisk);
  grub_unregister_command (cmd_kernel);
  grub_unregister_extcmd (cmd_splash);
  grub_unregister_command (cmd_kernel64);

  grub_cpu_xnu_fini ();
}
