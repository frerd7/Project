#
#  OSLG  -- Operating System Loader in GRUB 
#  Copyright (C) 2022 Free Software Foundation, Inc.
#
#  OSLG is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OSLG is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with OSLG.  If not, see <http://www.gnu.org/licenses/>.
#

import os
import sys

from libmkconfig import *

class Theme:
	def settings_gfx(self, file):
		setup = "\n setup_gui\nfi"
		file.write(setup)

	def directory_locale(self, file):
		locale = ["\n set locale_dir=$prefix/locale"
				 ," insmod gettext"
				 ]
		for line in locale:
			file.write(f"{line}\n")

	def __init__(self, file, root , THEME, probe):
		self.theme = f"{root}/themes"
		menu = ["function select_menu {"
			   ," if menu_popup -t template_popup theme_menu ; then"
			   ,"    free_config template_popup template_subitem menu class screen"
			   ,"    load_config ${prefix}/themes/${theme_name}/theme ${prefix}/themes/custom/theme_${theme_name}"
			   ,"    save_env theme_name"
			   ,"    menu_refresh"
			   ," fi\n}"
			   ,"\nfunction toggle_fold {"
			   ,"  if test -z $theme_fold ; then"
			   ,"     set theme_fold=1"
			   ,"  else"
			   ,"     set theme_fold="
			   ,"  fi"
			   ,"  save_env theme_fold"
			   ,"  menu_refresh\n}\n"
			   ]
		# < -------------- DEFINE THEME CONF -------->
		file.write("\n### DEFINE THEME CONF ###\n")
		if THEME == 'save':
			file.write("set theme_name=bgrt\n")
		else:
			file.write(f"set theme_name={THEME}\n")
			
		print(f"{tr('Theme used by default:')} {THEME}")

		for line in menu:
			file.write(f"{line}\n")
		file.write("if test -f ${prefix}/themes/${theme_name}/theme ; then")
		for dirs in os.listdir(self.theme):
			if os.path.isfile(dirs) == False:
				if not dirs == 'template' and not dirs == 'icons' and not dirs == 'conf.d':
								load = "\n load_string '+theme_menu {" + " -" + dirs + "   { command=\"set theme_name="+ dirs + "\"}}'"
								file.write(load)

		for dirs in os.listdir(self.theme):
			if os.path.isfile(dirs) == False:
				if dirs == 'conf.d':
					if os.path.isfile(dirs) == False:
						file.write("\n load_config ${prefix}/themes/conf.d/10_hotkey")
						file.write("\n load_config ${prefix}/themes/${theme_name}/theme ${prefix}/themes/custom/theme_${theme_name}")
			else:
				if os.path.isfile(dirs) == False:
					file.write("\n load_config " + "${prefix}/themes/${theme_name}/theme ${prefix}/themes/custom/theme_${theme_name}")			
		# < --------------- Setup UI ----------->
		self.settings_gfx(file)
		self.directory_locale(file)
		file.write("### END THEME CONF ###\n")

		if os.path.exists("/sys/firmware/acpi/bgrt/image") == True:
			if os.path.exists(f"{root}/themes/bgrt") == True:
				convert_bmp_to_png("/sys/firmware/acpi/bgrt/image", f"{root}/themes/bgrt/bgrt.png", 1.0)
