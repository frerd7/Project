/* getroot.c - Get root device */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <config.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#ifdef __CYGWIN__
# include <sys/fcntl.h>
# include <sys/cygwin.h>
# include <limits.h>
# define DEV_CYGDRIVE_MAJOR 98
#endif

#ifdef __GNU__
#include <hurd.h>
#include <hurd/lookup.h>
#include <hurd/fs.h>
#endif

#include <grub/util/misc.h>
#include <grub/util/hostdisk.h>
#include <grub/util/getroot.h>

static void
strip_extra_slashes (char *dir)
{
  char *p = dir;

  while ((p = strchr (p, '/')) != 0)
    {
      if (p[1] == '/')
	{
	  memmove (p, p + 1, strlen (p));
	  continue;
	}
      else if (p[1] == '\0')
	{
	  if (p > dir)
	    p[0] = '\0';
	  break;
	}

      p++;
    }
}

static char *
xgetcwd (void)
{
  size_t size = 10;
  char *path;

  path = xmalloc (size);
  while (! getcwd (path, size))
    {
      size <<= 1;
      path = xrealloc (path, size);
    }

  return path;
}

#if defined(__CYGWIN__) || defined(__MINGW32__)
static char *
convert_win32_path (char *winpath)
{
  int len = strlen (winpath);
  if (len > 2 && winpath[1] == ':')
    {
      len -= 2;
      memmove (winpath, winpath + 2, len + 1);
    }

  int i;
  for (i = 0; i < len; i++)
    if (winpath[i] == '\\')
      winpath[i] = '/';

  return xstrdup (winpath);
}
#endif

#ifdef __CYGWIN__
/* Convert POSIX path to Win32 path,
   remove drive letter, replace backslashes.  */
static char *
get_win32_path (const char *path)
{
  char winpath[PATH_MAX];
  cygwin_conv_to_full_win32_path (path, winpath);
  return convert_win32_path (winpath);
}
#endif

#if defined(__MINGW32__)
char *
grub_get_prefix (const char *dir, int host_dev __attribute__ ((unused)))
{
  char *saved_cwd;
  char *path;
  char *result;

  saved_cwd = xgetcwd ();

  if (chdir (dir) < 0)
    grub_util_error ("Cannot change directory to `%s'", dir);

  path = xgetcwd ();
  result = convert_win32_path (path);
  strip_extra_slashes (result);

  chdir (saved_cwd);
  free (saved_cwd);
  free (path);  
  return result;
}
#else
char *
grub_get_prefix (const char *dir, int host_dev)
{
  char *saved_cwd;
  char *abs_dir, *prev_dir;
  char *prefix;
  struct stat st, prev_st;

  /* Save the current directory.  */
  saved_cwd = xgetcwd ();

  if (chdir (dir) < 0)
    grub_util_error ("cannot change directory to `%s'", dir);

  abs_dir = xgetcwd ();
  strip_extra_slashes (abs_dir);
  if (host_dev)
    {
      if (chdir (saved_cwd) < 0)
	grub_util_error ("Cannot change directory to `%s'", saved_cwd);
      free (saved_cwd);
      return abs_dir;
    }
  prev_dir = xstrdup (abs_dir);

  if (stat (".", &prev_st) < 0)
    grub_util_error ("cannot stat `%s'", dir);

  if (! S_ISDIR (prev_st.st_mode))
    grub_util_error ("`%s' is not a directory", dir);

  while (1)
    {
      if (chdir ("..") < 0)
	grub_util_error ("cannot change directory to the parent");

      if (stat (".", &st) < 0)
	grub_util_error ("cannot stat current directory");

      if (! S_ISDIR (st.st_mode))
	grub_util_error ("current directory is not a directory???");

      if (prev_st.st_dev != st.st_dev || prev_st.st_ino == st.st_ino)
	break;

      free (prev_dir);
      prev_dir = xgetcwd ();
      prev_st = st;
    }

  strip_extra_slashes (prev_dir);
  prefix = xmalloc (strlen (abs_dir) - strlen (prev_dir) + 2);
  prefix[0] = '/';
  strcpy (prefix + 1, abs_dir + strlen (prev_dir));
  strip_extra_slashes (prefix);

  if (chdir (saved_cwd) < 0)
    grub_util_error ("cannot change directory to `%s'", dir);

#ifdef __CYGWIN__
  if (st.st_dev != (DEV_CYGDRIVE_MAJOR << 16))
    {
      /* Reached some mount point not below /cygdrive.
	 GRUB does not know Cygwin's emulated mounts,
	 convert to Win32 path.  */
      grub_util_info ("Cygwin prefix = %s", prefix);
      char * wprefix = get_win32_path (prefix);
      free (prefix);
      prefix = wprefix;
    }
#endif

  free (saved_cwd);
  free (abs_dir);
  free (prev_dir);

  grub_util_info ("prefix = %s", prefix);
  return prefix;
}
#endif

#ifdef __MINGW32__

static char *
find_root_device (const char *dir __attribute__ ((unused)),
                  dev_t dev __attribute__ ((unused)))
{
  return 0;
}

#elif ! defined(__CYGWIN__)

static char *
find_root_device (const char *dir, dev_t dev)
{
  DIR *dp;
  char *saved_cwd;
  struct dirent *ent;

  dp = opendir (dir);
  if (! dp)
    return 0;

  saved_cwd = xgetcwd ();

  grub_util_info ("changing current directory to %s", dir);
  if (chdir (dir) < 0)
    {
      free (saved_cwd);
      closedir (dp);
      return 0;
    }

  while ((ent = readdir (dp)) != 0)
    {
      struct stat st;

      /* Avoid:
	 - dotfiles (like "/dev/.tmp.md0") since they could be duplicates.
	 - dotdirs (like "/dev/.static") since they could contain duplicates.  */
      if (ent->d_name[0] == '.')
	continue;

      if (lstat (ent->d_name, &st) < 0)
	/* Ignore any error.  */
	continue;

      if (S_ISLNK (st.st_mode))
	/* Don't follow symbolic links.  */
	continue;

      if (S_ISDIR (st.st_mode))
	{
	  /* Find it recursively.  */
	  char *res;

	  res = find_root_device (ent->d_name, dev);

	  if (res)
	    {
	      if (chdir (saved_cwd) < 0)
		grub_util_error ("cannot restore the original directory");

	      free (saved_cwd);
	      closedir (dp);
	      return res;
	    }
	}

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
      if (S_ISCHR (st.st_mode) && st.st_rdev == dev)
#else
      if (S_ISBLK (st.st_mode) && st.st_rdev == dev)
#endif
	{
#ifdef __linux__
	  /* Skip device names like /dev/dm-0, which are short-hand aliases
	     to more descriptive device names, e.g. those under /dev/mapper */
	  if (ent->d_name[0] == 'd' &&
	      ent->d_name[1] == 'm' &&
	      ent->d_name[2] == '-' &&
	      ent->d_name[3] >= '0' &&
	      ent->d_name[3] <= '9')
	    continue;
#endif

	  /* Found!  */
	  char *res;
	  char *cwd;

	  cwd = xgetcwd ();
	  res = xmalloc (strlen (cwd) + strlen (ent->d_name) + 2);
	  sprintf (res, "%s/%s", cwd, ent->d_name);
	  strip_extra_slashes (res);
	  free (cwd);

	  /* /dev/root is not a real block device keep looking, takes care
	     of situation where root filesystem is on the same partition as
	     grub files */

	  if (strcmp(res, "/dev/root") == 0)
		continue;

	  if (chdir (saved_cwd) < 0)
	    grub_util_error ("cannot restore the original directory");

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    grub_util_error ("cannot restore the original directory");

  free (saved_cwd);
  closedir (dp);
  return 0;
}

#else /* __CYGWIN__ */

/* Read drive/partition serial number from mbr/boot sector,
   return 0 on read error, ~0 on unknown serial.  */
static unsigned
get_bootsec_serial (const char *os_dev, int mbr)
{
  /* Read boot sector.  */
  int fd = open (os_dev, O_RDONLY);
  if (fd < 0)
    return 0;
  unsigned char buf[0x200];
  int n = read (fd, buf, sizeof (buf));
  close (fd);
  if (n != sizeof(buf))
    return 0;

  /* Check signature.  */
  if (!(buf[0x1fe] == 0x55 && buf[0x1ff] == 0xaa))
    return ~0;

  /* Serial number offset depends on boot sector type.  */
  if (mbr)
    n = 0x1b8;
  else if (memcmp (buf + 0x03, "NTFS", 4) == 0)
    n = 0x048;
  else if (memcmp (buf + 0x52, "FAT32", 5) == 0)
    n = 0x043;
  else if (memcmp (buf + 0x36, "FAT", 3) == 0)
    n = 0x027;
  else
    return ~0;

  unsigned serial = *(unsigned *)(buf + n);
  if (serial == 0)
    return ~0;
  return serial;
}

static char *
find_cygwin_root_device (const char *path, dev_t dev)
{
  /* No root device for /cygdrive.  */
  if (dev == (DEV_CYGDRIVE_MAJOR << 16))
    return 0;

  /* Convert to full POSIX and Win32 path.  */
  char fullpath[PATH_MAX], winpath[PATH_MAX];
  cygwin_conv_to_full_posix_path (path, fullpath);
  cygwin_conv_to_full_win32_path (fullpath, winpath);

  /* If identical, this is no real filesystem path.  */
  if (strcmp (fullpath, winpath) == 0)
    return 0;

  /* Check for floppy drive letter.  */
  if (winpath[0] && winpath[1] == ':' && strchr ("AaBb", winpath[0]))
    return xstrdup (strchr ("Aa", winpath[0]) ? "/dev/fd0" : "/dev/fd1");

  /* Cygwin returns the partition serial number in stat.st_dev.
     This is never identical to the device number of the emulated
     /dev/sdXN device, so above find_root_device () does not work.
     Search the partition with the same serial in boot sector instead.  */
  char devpath[sizeof ("/dev/sda15") + 13]; /* Size + Paranoia.  */
  int d;
  for (d = 'a'; d <= 'z'; d++)
    {
      sprintf (devpath, "/dev/sd%c", d);
      if (get_bootsec_serial (devpath, 1) == 0)
	continue;
      int p;
      for (p = 1; p <= 15; p++)
	{
	  sprintf (devpath, "/dev/sd%c%d", d, p);
	  unsigned ser = get_bootsec_serial (devpath, 0);
	  if (ser == 0)
	    break;
	  if (ser != (unsigned)~0 && dev == (dev_t)ser)
	    return xstrdup (devpath);
	}
    }
  return 0;
}

#endif /* __CYGWIN__ */

char *
grub_guess_root_device (const char *dir)
{
  char *os_dev;
#ifdef __GNU__
  file_t file;
  mach_port_t *ports;
  int *ints;
  loff_t *offsets;
  char *data;
  error_t err;
  mach_msg_type_number_t num_ports = 0, num_ints = 0, num_offsets = 0, data_len = 0;
  size_t name_len;

  file = file_name_lookup (dir, 0, 0);
  if (file == MACH_PORT_NULL)
    return 0;

  err = file_get_storage_info (file,
			       &ports, &num_ports,
			       &ints, &num_ints,
			       &offsets, &num_offsets,
			       &data, &data_len);

  if (num_ints < 1)
    grub_util_error ("Storage info for `%s' does not include type", dir);
  if (ints[0] != STORAGE_DEVICE)
    grub_util_error ("Filesystem of `%s' is not stored on local disk", dir);

  if (num_ints < 5)
    grub_util_error ("Storage info for `%s' does not include name", dir);
  name_len = ints[4];
  if (name_len < data_len)
    grub_util_error ("Bogus name length for storage info for `%s'", dir);
  if (data[name_len - 1] != '\0')
    grub_util_error ("Storage name for `%s' not NUL-terminated", dir);

  os_dev = xmalloc (strlen ("/dev/") + data_len);
  memcpy (os_dev, "/dev/", strlen ("/dev/"));
  memcpy (os_dev + strlen ("/dev/"), data, data_len);

  if (ports && num_ports > 0)
    {
      mach_msg_type_number_t i;
      for (i = 0; i < num_ports; i++)
        {
	  mach_port_t port = ports[i];
	  if (port != MACH_PORT_NULL)
	    mach_port_deallocate (mach_task_self(), port);
        }
      munmap ((caddr_t) ports, num_ports * sizeof (*ports));
    }

  if (ints && num_ints > 0)
    munmap ((caddr_t) ints, num_ints * sizeof (*ints));
  if (offsets && num_offsets > 0)
    munmap ((caddr_t) offsets, num_offsets * sizeof (*offsets));
  if (data && data_len > 0)
    munmap (data, data_len);
  mach_port_deallocate (mach_task_self (), file);
#else /* !__GNU__ */
  struct stat st;

  if (stat (dir, &st) < 0)
    grub_util_error ("cannot stat `%s'", dir);

#ifdef __CYGWIN__
  /* Cygwin specific function.  */
  os_dev = find_cygwin_root_device (dir, st.st_dev);

#else

  /* This might be truly slow, but is there any better way?  */
  os_dev = find_root_device ("/dev", st.st_dev);
#endif
#endif /* !__GNU__ */

  return os_dev;
}

static int
grub_util_is_dmraid (const char *os_dev)
{
  if (! strncmp (os_dev, "/dev/mapper/nvidia_", 19))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/isw_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/hpt37x_", 19))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/hpt45x_", 19))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/via_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/lsi_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/pdc_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/jmicron_", 20))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/asr_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/sil_", 16))
    return 1;

  return 0;
}

int
grub_util_get_dev_abstraction (const char *os_dev __attribute__((unused)))
{
#ifdef __linux__
  /* Check for LVM.  */
  if (!strncmp (os_dev, "/dev/mapper/", 12)
      && ! grub_util_is_dmraid (os_dev)
      && strncmp (os_dev, "/dev/mapper/mpath", 17) != 0)
    return GRUB_DEV_ABSTRACTION_LVM;

  /* Check for RAID.  */
  if (!strncmp (os_dev, "/dev/md", 7))
    return GRUB_DEV_ABSTRACTION_RAID;
#endif

  /* No abstraction found.  */
  return GRUB_DEV_ABSTRACTION_NONE;
}

char *
grub_util_get_grub_dev (const char *os_dev)
{
  char *grub_dev;

  switch (grub_util_get_dev_abstraction (os_dev))
    {
    case GRUB_DEV_ABSTRACTION_LVM:

      {
	unsigned short i, len;
	grub_size_t offset = sizeof ("/dev/mapper/") - 1;

	len = strlen (os_dev) - offset + 1;
	grub_dev = xmalloc (len);

	for (i = 0; i < len; i++, offset++)
	  {
	    grub_dev[i] = os_dev[offset];
	    if (os_dev[offset] == '-' && os_dev[offset + 1] == '-')
	      offset++;
	  }
      }

      break;

    case GRUB_DEV_ABSTRACTION_RAID:

      if (os_dev[7] == '_' && os_dev[8] == 'd')
	{
	  /* This a partitionable RAID device of the form /dev/md_dNNpMM. */

	  char *p, *q;

	  p = strdup (os_dev + sizeof ("/dev/md_d") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/' && os_dev[8] == 'd')
	{
	  /* This a partitionable RAID device of the form /dev/md/dNNpMM. */

	  char *p, *q;

	  p = strdup (os_dev + sizeof ("/dev/md/d") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] >= '0' && os_dev[7] <= '9')
	{
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/' && os_dev[8] >= '0' && os_dev[8] <= '9')
	{
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md/") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else
	grub_util_error ("unknown kind of RAID device `%s'", os_dev);

      break;

    default:  /* GRUB_DEV_ABSTRACTION_NONE */
      grub_dev = grub_util_biosdisk_get_grub_dev (os_dev);
    }

  return grub_dev;
}

const char *
grub_util_check_block_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error ("cannot stat `%s'", blk_dev);

  if (S_ISBLK (st.st_mode))
    return (blk_dev);
  else
    return 0;
}

const char *
grub_util_check_char_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error ("cannot stat `%s'", blk_dev);

  if (S_ISCHR (st.st_mode))
    return (blk_dev);
  else
    return 0;
}

