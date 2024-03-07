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

#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/lib.h>
#include <grub/command.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

static grub_err_t
grub_cmd_settings_gui (struct grub_extcmd *cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{

  const char *gfxfont;
  gfxfont = grub_env_get ("gfxfont");
  if(gfxfont == NULL)
  {
     grub_env_set ("gfxfont", "Unifont Regular 16");
  }

/* load coreui module */
  grub_dl_load ("coreui");
  grub_errno = 0;
  grub_command_execute ("menu_region.text", 0, 0);
#if defined(GRUB_TARGET_I386)
  grub_dl_load ("vbe");
#endif
/* load renders video fb */
  grub_dl_load ("png");
  grub_dl_load ("jpg");
  grub_errno = 0;
/* controller ext */
  grub_command_execute ("menu_region.gfx", 0, 0);
  grub_command_execute ("controller.ext", 0, 0);

return 0;       
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(settings_gui)
{
   cmd =  grub_register_extcmd ("setup_gui", grub_cmd_settings_gui, GRUB_COMMAND_FLAG_BOTH,
	    0, N_("Settings GUI interface video mode"), 0);
   
}

GRUB_MOD_FINI(settings_gui)
{
  grub_unregister_extcmd (cmd);
}

