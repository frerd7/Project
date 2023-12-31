/* corecmd.c - critical commands which are registered in kernel */
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

#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/command.h>

static int
print_env (struct grub_env_var *env, void *closure __attribute__ ((unused)))
{
  grub_printf ("%s=%s\n", env->name, env->value);
  return 0;
}

/* set ENVVAR=VALUE */
static grub_err_t
grub_core_cmd_set (struct grub_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  char *var;
  char *val;

  if (argc < 1)
    {
      grub_env_iterate (print_env, 0);
      return 0;
    }

  var = argv[0];
  val = grub_strchr (var, '=');
  if (! val)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "not an assignment");

  val[0] = 0;
  grub_env_set (var, val + 1);
  val[0] = '=';

  return 0;
}

static grub_err_t
grub_core_cmd_unset (struct grub_command *cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       "no environment variable specified");

  grub_env_unset (argv[0]);
  return 0;
}

/* insmod MODULE */
static grub_err_t
grub_core_cmd_insmod (struct grub_command *cmd __attribute__ ((unused)),
		      int argc, char *argv[])
{
  char *p;
  grub_dl_t mod;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");

  p = grub_strchr (argv[0], '/');
  if (! p)
    mod = grub_dl_load (argv[0]);
  else
    mod = grub_dl_load_file (argv[0]);

  if (mod)
    grub_dl_ref (mod);

  return 0;
}

static int
grub_mini_print_devices (const char *name,
			 void *closure __attribute__ ((unused)))
{
  grub_printf ("(%s) ", name);

  return 0;
}

static int
grub_mini_print_files (const char *filename,
		       const struct grub_dirhook_info *info,
		       void *closure __attribute__ ((unused)))
{
  grub_printf ("%s%s ", filename, info->dir ? "/" : "");

  return 0;
}

/* ls [ARG] */
static grub_err_t
grub_core_cmd_ls (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char *argv[])
{
  if (argc < 1)
    {
      grub_device_iterate (grub_mini_print_devices, 0);
      grub_putchar ('\n');
      grub_refresh ();
    }
  else
    {
      char *device_name;
      grub_device_t dev;
      grub_fs_t fs;
      char *path;

      device_name = grub_file_get_device_name (argv[0]);
      dev = grub_device_open (device_name);
      if (! dev)
	goto fail;

      fs = grub_fs_probe (dev);
      path = grub_strchr (argv[0], ')');
      if (! path)
	path = argv[0];
      else
	path++;

      if (! path && ! device_name)
	{
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid argument");
	  goto fail;
	}

      if (! path)
	{
	  if (grub_errno == GRUB_ERR_UNKNOWN_FS)
	    grub_errno = GRUB_ERR_NONE;

	  grub_printf ("(%s): Filesystem is %s.\n",
		       device_name, fs ? fs->name : "unknown");
	}
      else if (fs)
	{
	  (fs->dir) (dev, path, grub_mini_print_files, 0);
	  grub_putchar ('\n');
	  grub_refresh ();
	}

    fail:
      if (dev)
	grub_device_close (dev);

      grub_free (device_name);
    }

  return grub_errno;
}

void
grub_register_core_commands (void)
{
  grub_register_command ("set", grub_core_cmd_set,
			 "[ENVVAR=VALUE]", "Set an environment variable.");
  grub_register_command ("unset", grub_core_cmd_unset,
			 "ENVVAR", "Remove an environment variable.");
  grub_register_command ("ls", grub_core_cmd_ls,
			 "[ARG]", "List devices or files.");
  grub_register_command ("insmod", grub_core_cmd_insmod,
			 "MODULE", "Insert a module.");
}
