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
import subprocess
import re

from libmkconfig import *
from traslate import *

def unicode_string(string):
    s = string.strip().decode("utf-8")
    return s

class OSPROBER:
    def hurd_entry(file, probe, longname, device, label):
         hurddevice = subprocess.check_output(f"{probe} --device {device} --target=drive", shell=True)
         machdevice = subprocess.check_output(f"echo {unicode_string(hurddevice)} | tr -d '()' | tr , s", shell=True)
         hurdfs = subprocess.check_output(probe + " --device " + device + " --target=fs", shell=True) 
         hurd_fs = unicode_string(hurdfs) + "fs"
         hhurd = ["\n\tmultiboot /boot/gnumach.gz root=device:" + unicode_string(machdevice), 
             "\n\tmodule /hurd/" + hurd_fs + ".static " + hurd_fs +" --readonly\\", 
             "\n\t           --multiboot-command-line='${kernel-command-line}'\\", 
             "\n\t           --host-priv-port='${host-port}'\\", 
             "\n\t           --device-master-port='${device-port}'\\", 
             "\n\t           --exec-server-task='${exec-task}' -T typed '${root}'\\", 
             "\n\t           '$(task-create)' '$(task-resume)'", 
             "\n\tmodule /lib/ld.so.1 exec /hurd/exec '$(exec-task=task-create)'"]
         create_menu_entry(file, longname, device, label, 1)
         access_to_device(file, probe, unicode_string(device), 0, 0)
         for i in hhurd:
              file.write(i)

    def osx_entry(file, probe, longname, device, label, xnu_kernel, prefix):
         osx_uuid = subprocess.check_output(f"{probe} --device {device} --target=fs_uuid",shell=True)
         video = subprocess.check_output(f"head -n 1 {prefix}/video.lst || true",shell=True)
         VIDEO_BACKED = unicode_string(video)
         osxuuid = unicode_string(osx_uuid)
         create_menu_entry(file, longname, device, label, 1)
         access_to_device(file, probe, unicode_string(device), 0, 0)
         arg = ["\n\tset do_resume=0", 
           "\n\tif [ /var/vm/sleepimage -nt10 / ]; then", 
           "\n\t  if xnu_resume /var/vm/sleepimage; then", 
           "\n\t     set do_resume=1",
           "\n\t  fi", 
           "\n\tfi", 
           "\n\tif [ \$do_resume == 0 ]; then", 
           "\n\t xnu_uuid " + osxuuid + "uuid",
           "\n\t if [ -f /Extra/DSDT.aml ]; then",
           "\n\t       acpi -e /Extra/DSDT.aml", 
           "\n\t fi", 
           "\n\t " + xnu_kernel + " /mach_kernel boot-uuid=${uuid} rd=*uuid",
           "\n\t if [ /System/Library/Extensions.mkext -nt /System/Library/Extensions ]; then", 
           "\n\t      xnu_mkext /System/Library/Extensions.mkext", 
           "\n\t else", 
           "\n\t      xnu_kextdir /System/Library/Extensions", 
           "\n\t fi", 
	       "\n\t if [ -f /Extra/Extensions.mkext ]; then", 
           "\n\t      xnu_mkext /Extra/Extensions.mkext", 
           "\n\t fi", 
           "\n\t if [ -d /Extra/Extensions ]; then", 
           "\n\t      xnu_kextdir /Extra/Extensions", 
           "\n\t fi", 
           "\n\t if [ -f /Extra/devprop.bin ]; then", 
           "\n\t      xnu_devprop_load /Extra/devprop.bin", 
           "\n\t fi", 
           "\n\t if [ -f /Extra/splash.jpg ]; then", 
           "\n\t      insmod jpeg", 
           "\n\t      xnu_splash /Extra/splash.jpg", 
           "\n\t fi", 
           "\n\t if [ -f /Extra/splash.png ]; then", 
           "\n\t       insmod png", 
           "\n\t       xnu_splash /Extra/splash.png", 
           "\n\t fi", 
           "\n\t if [ -f /Extra/splash.tga ]; then", 
           "\n\t      insmod tga", 
           "\n\t      xnu_splash /Extra/splash.tga", 
           "\n\t fi", 
           "\n\tfi", "\n}"]
         file.write("\n\tinsmod " + VIDEO_BACKED)
         for i in arg:
              file.write(i)

    def __init__(self, file, probe, prefix):
        self.probe = subprocess.check_output("os-prober | tr ' ' '^' | paste -s -d ' '", shell=True)
        self.os_pro = unicode_string(self.probe)
        if self.os_pro == None:
              exit()
        
        self.device = subprocess.check_output(f"echo {self.os_pro} | cut -d ':' -f 1", shell=True)
        self.longname = subprocess.check_output(f"echo {self.os_pro} | cut -d ':' -f 2", shell=True)
        self.label = subprocess.check_output(f"echo {self.os_pro}  | cut -d ':' -f 3", shell=True)
        self.boot = subprocess.check_output(f"echo {self.os_pro} | cut -d ':' -f 4", shell=True)
        
        self.oslabel = self.label
        if unicode_string(self.label) == None:
            self.oslabel = "unknown"
        
        print(f"{tr('Found')} " + unicode_string(self.longname).replace("^", " ") + f" {tr('on')} " + unicode_string(self.device))
        # <----------------- DEVICE PROBER ------------>
        self.bootnew = unicode_string(self.boot)
        self.alg = unicode_string(self.longname)
        if self.bootnew == 'chain' or self.bootnew == 'efi':
            file.write("\n### DEFINE DEVICE OS PROBER WINDOWS ###\n")
            if self.bootnew == 'efi':
                 match = re.match(r'^/dev/[^/]+', unicode_string(self.device))
                 path = match.group(0) 
                 self.device = path.replace('@', '')
            else:
                 self.device = unicode_string(self.device)   
            create_menu_entry(file, self.longname, self.device, self.label, 1)
            access_to_device(file, probe, self.device, 0, 0)
            if self.alg == "Windows^Vista":
                      pass
            elif self.alg == "Windows^7":
                      pass
            else:
                file.write("\n\tdrivemap -s (hd0) ${root}") 
                file.write("\n\tchainloader +1\n")  
                file.write("}\n")  
            file.write("### END DEVICE OS PROBER WINDOWS ###\n")
        elif self.bootnew == 'macosx':
             file.write("\n### DEFINE OS PROBER MACOSX ###\n")
             self.osx_entry(file, probe, self.longname, self.device, self.label, "xnu_kernel 32", prefix)
             self.osx_entry(file, probe, self.longname, self.device, self.label, "xnu_kernel 64", prefix)         
             file.write("\n### END OS PROBER MACOSX ###\n")
        elif self.bootnew == 'hurd':
             file.write("\n### DEFINE OS PROBER GNUHURD ###\n") 
             self.hurd_entry(file, probe, self.longname, self.device, self.label)
             file.write("### END OS PROBER GNUHURD ###\n")
