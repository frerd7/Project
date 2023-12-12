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

import sys
import os
import re
import subprocess

from libmkconfig import *
from traslate import *

class VMLinux:
    def find_osname(self):
         os = subprocess.check_output("lsb_release -i -s 2> /dev/null || echo Debian",shell=True)
         return unicode_string(os)
    
    def recovery_menu(self, file, version, files, recovery):
              if recovery == True:
                 osname = f"{self.find_osname()}, from Linux{version} (Recovery Mode)"
                 # check recovery menu
                 if os.path.isfile("/lib/recovery-mode/recovery-menu") == True:
                    CMDLINE_LINUX_RECOVERY="recovery nomodeset"
                 else:
                    CMDLINE_LINUX_RECOVERY="single"

                 create_menu_entry(file, osname, "", self.find_osname(), 0)
                 access_to_device(file, self.probe, self.dir, 1, 0)
                 file.write(f"\n\tlinux {self.pref[0]}/{files} root={access_to_device_uuid(self.probe, self.dir)} ro {CMDLINE_LINUX_RECOVERY}")
                 self.initrd = ["initrd.img" + version, "initrd" + version + ".img", "initrd" + version, "initramfs" + version + ".img"] 
                 for i in self.initrd: 
                     if os.path.isfile(f"{self.dir}/{i}"):
                        file.write(f"\n\tinitrd {self.pref[0]}/{i}\n") 
                        file.write("}\n")

    def __init__(self, dir, cmdline, file, recovery, probe):
        self.dir = dir
        self.probe = probe
        self.pref = re.findall(r"[*/;\n]\w+", self.dir)
        file.write("\n### DEFINE LINUX OS ###\n")
        for files in os.listdir(self.dir):
           if files.startswith("vmlinuz-") or files.startswith("vmlinux-"):
                    print(f"{tr('Found linux image')}: {self.dir}/{files}")
                    new = re.findall(r"[*-;\n]+[*-0-9]\w+", files)  
                    version = ' '.join([str(item) for item in new])
                    oslabel = f"{self.find_osname()}, from Linux{version}"
            # <---------- create menu entry --------->
                    create_menu_entry(file, oslabel, "", self.find_osname(), 0)
                    access_to_device(file, self.probe, self.dir, 1, 0)
            # <--------------- file access ----------->
                    file.write(f"\n\tlinux {self.pref[0]}/{files} root={access_to_device_uuid(self.probe, dir)} ro {cmdline}")
                    self.initrd = ["initrd.img" + version, "initrd" + version + ".img", "initrd" + version, "initramfs" + version + ".img"] 
                    for i in self.initrd: 
                          if os.path.isfile(f"{dir}/{i}"):
                           print(f"{tr('Found intrd image')}: {self.dir}/{i}")
                           file.write(f"\n\tinitrd {self.pref[0]}/{i}\n")
                           file.write("}\n")
                           self.recovery_menu(file, version, files, recovery)
        file.write("\n### END LINUX OS ###\n")