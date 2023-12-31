/* terminfo.c - simple terminfo module */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007  Free Software Foundation, Inc.
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

/*
 * This file contains various functions dealing with different
 * terminal capabilities. For example, vt52 and vt100.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/term.h>
#include <grub/terminfo.h>
#include <grub/tparm.h>
#include <grub/command.h>

GRUB_EXPORT(grub_terminfo_cls);
GRUB_EXPORT(grub_terminfo_cursor_on);
GRUB_EXPORT(grub_terminfo_cursor_off);
GRUB_EXPORT(grub_terminfo_gotoxy);
GRUB_EXPORT(grub_terminfo_reverse_video_on);
GRUB_EXPORT(grub_terminfo_reverse_video_off);

struct terminfo
{
  char *name;

  char *gotoxy;
  char *cls;
  char *reverse_video_on;
  char *reverse_video_off;
  char *cursor_on;
  char *cursor_off;
};

static struct terminfo term;

/* Get current terminfo name.  */
char *
grub_terminfo_get_current (void)
{
  return term.name;
}

/* Free *PTR and set *PTR to NULL, to prevent double-free.  */
static void
grub_terminfo_free (char **ptr)
{
  grub_free (*ptr);
  *ptr = 0;
}

/* Set current terminfo type.  */
grub_err_t
grub_terminfo_set_current (const char *str)
{
  /* TODO
   * Lookup user specified terminfo type. If found, set term variables
   * as appropriate. Otherwise return an error.
   *
   * How should this be done?
   *  a. A static table included in this module.
   *     - I do not like this idea.
   *  b. A table stored in the configuration directory.
   *     - Users must convert their terminfo settings if we have not already.
   *  c. Look for terminfo files in the configuration directory.
   *     - /usr/share/terminfo is 6.3M on my system.
   *     - /usr/share/terminfo is not on most users boot partition.
   *     + Copying the terminfo files you want to use to the grub
   *       configuration directory is easier then (b).
   *  d. Your idea here.
   */

  /* Free previously allocated memory.  */
  grub_terminfo_free (&term.name);
  grub_terminfo_free (&term.gotoxy);
  grub_terminfo_free (&term.cls);
  grub_terminfo_free (&term.reverse_video_on);
  grub_terminfo_free (&term.reverse_video_off);
  grub_terminfo_free (&term.cursor_on);
  grub_terminfo_free (&term.cursor_off);

  if (grub_strcmp ("vt100", str) == 0)
    {
      term.name              = grub_strdup ("vt100");
      term.gotoxy            = grub_strdup ("\e[%i%p1%d;%p2%dH");
      term.cls               = grub_strdup ("\e[H\e[J");
      term.reverse_video_on  = grub_strdup ("\e[7m");
      term.reverse_video_off = grub_strdup ("\e[m");
      term.cursor_on         = grub_strdup ("\e[?25h");
      term.cursor_off        = grub_strdup ("\e[?25l");
      return grub_errno;
    }

  return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown terminfo type");
}

/* Wrapper for grub_putchar to write strings.  */
static void
putstr (const char *str, grub_term_output_t oterm)
{
  while (*str)
    grub_putcode (*str++, oterm);
}

/* Move the cursor to the given position starting with "0".  */
void
grub_terminfo_gotoxy (grub_uint8_t x, grub_uint8_t y, grub_term_output_t oterm)
{
  putstr (grub_terminfo_tparm (term.gotoxy, y, x), oterm);
}

/* Clear the screen.  */
void
grub_terminfo_cls (grub_term_output_t oterm)
{
  putstr (grub_terminfo_tparm (term.cls), oterm);
}

/* Set reverse video mode on.  */
void
grub_terminfo_reverse_video_on (grub_term_output_t oterm)
{
  putstr (grub_terminfo_tparm (term.reverse_video_on), oterm);
}

/* Set reverse video mode off.  */
void
grub_terminfo_reverse_video_off (grub_term_output_t oterm)
{
  putstr (grub_terminfo_tparm (term.reverse_video_off), oterm);
}

/* Show cursor.  */
void
grub_terminfo_cursor_on (grub_term_output_t oterm)
{
  putstr (grub_terminfo_tparm (term.cursor_on), oterm);
}

/* Hide cursor.  */
void
grub_terminfo_cursor_off (grub_term_output_t oterm)
{
  putstr (grub_terminfo_tparm (term.cursor_off), oterm);
}

/* GRUB Command.  */

static grub_err_t
grub_cmd_terminfo (grub_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  if (argc == 0)
  {
    grub_printf ("Current terminfo type: %s\n", grub_terminfo_get_current());
    return GRUB_ERR_NONE;
  }
  else if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "too many parameters");
  else
    return grub_terminfo_set_current (args[0]);
}

static grub_command_t cmd;

GRUB_MOD_INIT(terminfo)
{
  cmd = grub_register_command ("terminfo", grub_cmd_terminfo,
			       "[TERM]", "Set terminfo type.");
  grub_terminfo_set_current ("vt100");
}

GRUB_MOD_FINI(terminfo)
{
  grub_unregister_command (cmd);
}
