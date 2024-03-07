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
import locale
import shutil

from traslate import *

def check_root():
	if os.geteuid() != 0:
         raise Exception(f"{tr(f'The program can only be used in root mode, use sudo {sys.argv[0]} to continue')}")

def unicode_string(string):
    s = string.strip().decode("utf-8")
    return s

def access_to_device_uuid(probe, device):
    fs_uuid = subprocess.check_output(f"{probe} --target=fs_uuid {device}",shell=True)
    uuid = f"UUID={unicode_string(fs_uuid)}"
    return uuid

def create_menu_entry(file, longname, device, label, opt):
  if opt == 1:
    oslabel = unicode_string(label)
    entry = ["menuentry \""
             , unicode_string(longname).replace("^", " ")
             , " (on ", device, ")\" "
             , "--class "
             , oslabel.lower()
             , " --class os {\n"]
    for entryy in entry:
        file.write(entryy)
  else:
    oslabel = label
    entry = ["menuentry \""
             , longname, "\" "
             , "--class "
             , oslabel.lower()
             , " --class gnu-linux "
             , "--class gnu"
             ,  " --class os --group group_main {\n"]
    
    for entryy in entry:
        file.write(entryy)

def device_map():
   if shutil.which("x86_64-oslg-mkdevicemap") is not None:
       devicemap = "x86_64-oslg-mkdevicemap"
   elif shutil.which("i386-oslg-mkdevicemap") is not None:
       devicemap = "i386-oslg-mkdevicemap"
   else:
	   raise Exception(f"{tr('The *{platform}-oslg-devicemap command was not found on the system.')}")

   subprocess.run(f"{devicemap} -m /tmp/device.map", shell=True)


def access_to_device(file, probe, device, opt, tab):
    # device map 
    device_map()

    if opt == 1:
        fs = subprocess.check_output(f"{probe} --target=fs {device} -m /tmp/device.map", shell=True)
        root = subprocess.check_output(f"{probe} --target=drive {device} -m /tmp/device.map", shell=True)
        uid =  subprocess.check_output(f"{probe} --target=fs_uuid {device} -m /tmp/device.map", shell=True) 
        uuid =  unicode_string(uid)
        rxx = unicode_string(root)
        abstract = unicode_string(fs)
    else:
        fs = subprocess.check_output(f"{probe} --device {device} --target=fs -m /tmp/device.map", shell=True)
        root = subprocess.check_output(f"{probe} --device {device} --target=drive -m /tmp/device.map", shell=True)
        uid = subprocess.check_output(f"{probe} --device {device} --target=fs_uuid -m /tmp/device.map",shell=True)
        uuid =  unicode_string(uid)
        rxx = unicode_string(root)
        abstract = unicode_string(fs)
    
    if tab == 0:
       file.write(f"\tinsmod {abstract}")
       file.write(f"\n\tset root='{rxx} '")
    else:
       file.write(f"\n insmod {abstract}")
       file.write(f"\n set root='{rxx}'")

    if uuid == None:
        "Not UIID FOUND"
    else:
        if tab == 0:
           file.write(f"\n\tsearch --no-floppy --fs-uuid --set {uuid}")  
        else:
           file.write(f"\n search --no-floppy --fs-uuid --set {uuid}")

def convert_bmp_to_png(input_path, output_path, alpha_multiplier=0.5):
    try:
        with Image.open(input_path) as img:
            rgba_img = img.convert("RGBA")
            alpha = rgba_img.split()[3]

            new_alpha = alpha.point(lambda p: p * alpha_multiplier)
            rgba_img.putalpha(new_alpha)
            rgba_img.save(output_path)
    except Exception as e:
        print(f"Error: {e}")    

