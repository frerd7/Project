/*
 *  OSLG  -- Operating System Loader in GRUB 
 *  Copyright (C) 2022 Free Software Foundation, Inc.
 *
 *  OSLG is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OSLG is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OSLG.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/i18n.h>

static grub_err_t
grub_cmd_biosetup (grub_command_t cmd __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
    /* reboot into BIOS */
     __asm__ __volatile__("outb %0, %1" : : "a"(0x80), "Nd"(0x70));
    __asm__ __volatile__("outb %0, %1" : : "a"(0x01), "Nd"(0x64));
    return 0;
}

static grub_command_t cmd = NULL;

GRUB_MOD_INIT (biosetup)
{
    cmd = grub_register_command ("biosetup", grub_cmd_biosetup, NULL,
				 "Reboot into BIOS setup menu.");

}

GRUB_MOD_FINI (biosetup)
{
  if (cmd)
    grub_unregister_command (cmd);
}
