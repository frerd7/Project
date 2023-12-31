/* crypto.c - support crypto autoload */
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

#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/crypto.h>
#include <grub/normal.h>
#include <grub/file.h>
#include <grub/lib.h>

struct load_spec
{
  struct load_spec *next;
  char *name;
  char *modname;
};

struct load_spec *crypto_specs = NULL;

static void
grub_crypto_autoload (const char *name)
{
  struct load_spec *cur;
  grub_dl_t mod;

  for (cur = crypto_specs; cur; cur = cur->next)
    if (grub_strcasecmp (name, cur->name) == 0)
      {
	mod = grub_dl_load (cur->modname);
	if (mod)
	  grub_dl_ref (mod);
	grub_errno = GRUB_ERR_NONE;
      }
}

static void
grub_crypto_spec_free (void)
{
  struct load_spec *cur, *next;
  for (cur = crypto_specs; cur; cur = next)
    {
      next = cur->next;
      grub_free (cur->name);
      grub_free (cur->modname);
      grub_free (cur);
    }
  crypto_specs = NULL;
}


/* Read the file crypto.lst for auto-loading.  */
void
read_crypto_list (void)
{
  const char *prefix;
  char *filename;
  grub_file_t file;
  char *buf = NULL;

  prefix = grub_env_get ("prefix");
  if (!prefix)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  filename = grub_xasprintf ("%s/crypto.lst", prefix);
  if (!filename)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  file = grub_file_open (filename);
  grub_free (filename);
  if (!file)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  /* Override previous crypto.lst.  */
  grub_crypto_spec_free ();

  for (;; grub_free (buf))
    {
      char *p, *name;
      struct load_spec *cur;

      buf = grub_getline (file);

      if (! buf)
	break;

      name = buf;

      p = grub_strchr (name, ':');
      if (! p)
	continue;

      *p = '\0';
      while (*++p == ' ')
	;

      cur = grub_malloc (sizeof (*cur));
      if (!cur)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}

      cur->name = grub_strdup (name);
      if (! name)
	{
	  grub_errno = GRUB_ERR_NONE;
	  grub_free (cur);
	  continue;
	}

      cur->modname = grub_strdup (p);
      if (! cur->modname)
	{
	  grub_errno = GRUB_ERR_NONE;
	  grub_free (cur);
	  grub_free (cur->name);
	  continue;
	}
      cur->next = crypto_specs;
      crypto_specs = cur;
    }

  grub_file_close (file);

  grub_errno = GRUB_ERR_NONE;

  grub_crypto_autoload_hook = grub_crypto_autoload;
}
