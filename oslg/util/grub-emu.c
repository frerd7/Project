/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>

#include <grub/mm.h>
#include <grub/setjmp.h>
#include <grub/fs.h>
#include <grub/util/hostdisk.h>
#include <grub/dl.h>
#include <grub/util/console.h>
#include <grub/util/misc.h>
#include <grub/kernel.h>
#include <grub/normal.h>
#include <grub/util/getroot.h>
#include <grub/env.h>
#include <grub/partition.h>
#include <grub/i18n.h>

#ifdef HAVE_SDL_SDL_H
#include <SDL/SDL.h>
#endif

#include <grub_emu_init.h>

#define ENABLE_RELOCATABLE 0
#include "progname.h"

/* Used for going back to the main function.  */
static jmp_buf main_env;

/* Store the prefix specified by an argument.  */
static char *prefix = NULL;

struct grub_module_info grub_modinfo;
char grub_code_start[0];

grub_addr_t
grub_arch_modules_addr (void)
{
  return 0;
}

grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  (void) ehdr;

  return GRUB_ERR_BAD_MODULE;
}

grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  (void) mod;
  (void) ehdr;

  return GRUB_ERR_BAD_MODULE;
}

void
grub_reboot (void)
{
  longjmp (main_env, 1);
}

void
grub_halt (
#ifdef GRUB_MACHINE_PCBIOS
	   int no_apm __attribute__ ((unused))
#endif
	   )
{
  grub_reboot ();
}

void
grub_machine_init (void)
{
}

void
grub_machine_set_prefix (void)
{
  grub_env_set ("prefix", prefix);
  free (prefix);
  prefix = 0;
}

void
grub_machine_fini (void)
{
  grub_console_fini ();
}

void
read_command_list (void)
{
}


static struct option options[] =
  {
    {"root-device", required_argument, 0, 'r'},
    {"directory", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    { 0, 0, 0, 0 }
  };

static int
usage (int status)
{
  if (status)
    fprintf (stderr,
	     "Try `%s --help' for more information.\n", program_name);
  else
    printf (
      "Usage: %s [OPTION]...\n"
      "\n"
      "OSLG Theme emulator.\n"
      "\n"
      "  -r, --root-device=DEV     use DEV as the root device [default=guessed]\n"
      "  -d, --directory=DIR       use GRUB files in the directory DIR\n"
      "  -v, --verbose             print verbose messages\n"
      "  -h, --help                display this message and exit\n"
      "  -V, --version             print version information and exit\n"
      "\n"
      "Report bugs to <%s>.\n", program_name, PACKAGE_BUGREPORT);
  return status;
}


int
main (int argc, char *argv[])
{
  char *root_dev = 0;
  char *dir = 0;
  int opt;
  char cwd[PATH_MAX];

  set_program_name (argv[0]);

  grub_util_init_nls ();

  while ((opt = getopt_long (argc, argv, "r:d:m:v:hV", options, 0)) != -1)
    switch (opt)
      {
      case 'r':
        root_dev = optarg;
        break;
      case 'd':
        dir = optarg;
        break;
      case 'v':
        verbosity++;
        break;
      case 'h':
        return usage (0);
      case 'V':
        printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
        return 0;
      default:
        return usage (1);
      }

  if (optind < argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind]);
      return usage (1);
    }

  signal (SIGINT, SIG_IGN);
  grub_console_init ();

  grub_init_all ();

  /* Make sure that there is a root device.  */
  if (!root_dev)
    {
       root_dev = "host";
    }

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
       dir = (char*) cwd;
   }

  dir = grub_get_prefix (dir, ! strcmp (root_dev, "host"));
  prefix = xmalloc (strlen (root_dev) + 2 + strlen (dir) + 1);
  sprintf (prefix, "(%s)%s", root_dev, dir);
  free (dir);  

  /* Start GRUB!  */
  if (setjmp (main_env) == 0)
    grub_main ();

  grub_fini_all ();

  grub_machine_fini ();

  return 0;
}
