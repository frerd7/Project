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

import subprocess
import sys
import os
import shutil
import argparse
import platform

from lib import *

new_dir = "/usr/local/share/oslg"

# mod32 i386
module32_dir1 = "/usr/local/lib/i386-oslg/i386-pc"
module32_dir2 = "/usr/lib/i386-oslg/i386-pc"

# mod64 x64
module64_dir1 = "/usr/local/lib/x86_64-oslg/x86_64-efi"
module64_dir2 = "/usr/lib/x86_64-oslg/x86_64-efi"

# prefix
osg_prefix = "/boot/oslg"
share_dir = "/usr/share/oslg"

class BIOS:
	def __init__(self, root, prefix, mod, share_dir):
#prefix
		self.prefix = f"{root}{prefix}"
#comand
		self.probe = "i386-oslg-probe"
		self.mkdevice = "i386-oslg-mkdevicemap"
		self.edit = "i386-oslg-editenv "
		self.devicemap = f"{self.prefix}/device.map"
		self.setup = "i386-oslg-setup "
		self.mkimage = "i386-oslg-mkimage "
		self.mkconfig = "oslg-mkconfig"
#files
		self.cfg = f"{self.prefix}/core.img"
		self.p = Probe(root, self.probe)

		if not self.p.device('type') == 'ext2':
			raise Exception(f"{self.p.device('type')}, {tr('Device partition not supported for system BIOS/UEFI')}")

		if os.path.exists(f"{root}/boot") == False:
			os.mkdir(f"{root}/boot")
			os.mkdir(f"{self.prefix}")

		self.dev = determine_device_dev(root, self.probe)

		if os.path.exists(new_dir) == True:
			share_dir = new_dir
# device path info		
		print(f"{tr('Device:')} {self.p.device('device')}")
		print(f"UUID: {self.p.device('uuid')}")
			# <-------------- make device ---------->
		DeviceMap(root, prefix, self.mkdevice)
		init_cfg(mod, f"{self.prefix}", 0)
		Add_Modules(mod, f"{root}{prefix}", share_dir, self.edit)
			# <--------------- create img ----------> 
		subprocess.run(f"{self.mkimage} -d {self.prefix} -o {self.cfg}", shell=True)
		subprocess.run(f"{self.mkconfig} --root={root} -o {self.prefix}/oslg.cfg --theme_dir {self.prefix} --gfxmode 1280x1024", shell=True)
		subprocess.run(f"{self.setup} --force --directory={self.prefix} -m {self.devicemap}{self.dev}", shell=True)
		print(f"{tr('Installation finished. No error reported.')}")


class UEFI:
	def __init__(self, root, prefix, mod, share_dir):
# command
		self.probe = "x86_64-oslg-probe"
		self.edit = "x86_64-oslg-editenv "
		self.mkimage = "x86_64-oslg-mkimage "
		self.comand = "mkfs.ext2 "
		self.mkconfig = "x86_64-oslg-mkconfig"
# path
		self.memdisk = "/tmp/memdisk.img"
		self.boot = "/mnt"
		self.p = Probe(root, self.probe)
			
		if not self.p.device('type') == 'fat':
			raise Exception(f"{self.p.device('type')}, {tr('Device partition not supported for system BIOS/UEFI')}")

# run setup device	    
		subprocess.run(f"dd if=/dev/zero of={self.memdisk} bs=8000k count=1", stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
		subprocess.run(f"{self.comand} -F -m0 {self.memdisk}", stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=True)
		subprocess.run(f"mount -t ext2 -o loop {self.memdisk} {self.boot}", stdout=subprocess.DEVNULL, shell=True)		
# file
		if os.path.exists(f"{self.boot}/boot") == False:
			os.mkdir(f"{self.boot}/boot")
			os.mkdir(f"{self.boot}{prefix}") 

		if os.path.exists(f"{root}/EFI/BOOT") == False:
			os.mkdir(f"{root}/EFI")
			os.mkdir(f"{root}/EFI/BOOT")

		if os.path.exists(new_dir) == True:
			share_dir = new_dir
# info
		print(f"{tr('Device:')} {self.p.device('device')}")
		print(f"UUID: {self.p.device('uuid')}")
         
# install to device
		init_cfg(mod, f"{self.boot}{prefix}", 1)
		Add_Modules(mod, f"{self.boot}{prefix}", share_dir, self.edit)
		subprocess.run(f"{self.mkconfig} --root=/ -o {self.boot}{prefix}/oslg.cfg --theme_dir {self.boot}{prefix}", shell=True)
		subprocess.run("umount /mnt", shell=True)
		subprocess.run(f"{self.mkimage} -d {mod} -m {self.memdisk} -p \"\" -o {root}/EFI/BOOT/bootx64.efi", shell=True) 
		print(f"{tr('Installation finished. No error reported.')}")
	
def options(parser):
	parser.add_argument("-r", "--root", nargs='?', action='store', help=f"{tr('Install OSLG in a specific directory.')}")
	parser.add_argument("-d", "--directory", nargs='?', action='store', help=f"{tr('Module Specific Directory.')}")
	parser.add_argument("-m", "--machine", nargs='?',action='store', help=f"{tr('Supported machines i386 (x32), x86_64 (x64).')}")

if __name__ == "__main__":	
	parser = argparse.ArgumentParser(description=f"{tr('OSLG Bootloader Installer.')}")
	# check root 
	check_root()
	# options 
	options(parser)
	# argparse
	args = parser.parse_args()
	 
# direcory 
	if args.directory == None:
		if args.machine == None:
			if platform.machine() == 'i686':
				if os.path.exists(module32_dir1) == True:
					mod32 = module32_dir1
				elif os.path.exists(module32_dir2) == True:
					mod32 = module32_dir2
				else:
					raise Exception(f"{tr('Cannot access modules files directory.')}")
			elif platform.machine() == 'x86_64': 
				if os.path.exists(module64_dir1) == True:
					mod64 = module64_dir1
				elif os.path.exists(module64_dir2):
					mod64 = module64_dir2
				else:
					raise Exception(f"{tr('Cannot access modules files directory.')}")
		elif args.machine == 'x64':
			if os.path.exists(module64_dir1) == True:
				mod64 = module64_dir1
			elif os.path.exists(module64_dir2):
				mod64 = module64_dir2
			else:
				raise Exception(f"{tr('Cant access the modules directory, try reinstalling the .mod files or type -d or --directory path')}")
		elif args.machine == 'x32':
			if os.path.exists(module32_dir1) == True:
				mod32 = module32_dir1
			elif os.path.exists(module32_dir2) == True:
				mod32 = module32_dir2
			else:
				raise Exception(f"{tr('Cannot access modules files directory.')}")
	else:
		if args.machine == None:
			if platform.machine() == 'i686': 
				mod32 = args.directory 
			elif platform.machine() == 'x86_64':
				mod64 = args.directory
		elif args.machine == 'x64':
			mod64 = args.directory
# object
	if args.root == None: 
		raise Exception(f"{tr('No device directory specified, use -r or --root directory')}")
	else:
		if args.machine == None:
			if platform.machine() == 'i686':
				print(f"{tr('Installing OSLG (i386) Version => 2.0-beta-src2 ...')}")
				BIOS(args.root, osg_prefix , mod32, share_dir) 
			elif platform.machine() == 'x86_64':
				print(f"{tr('Installing OSLG (x86_64) Version => 2.0-beta-src2 ...')}")
				UEFI(args.root, osg_prefix , mod64, share_dir) 
			else:
				raise Exception(f"{tr('Platform Device not supported')}")
		elif args.machine == 'x64':
			print(f"{tr('Installing OSLG (x86_64) Version => 2.0-beta-src2 ...')}")
			UEFI(args.root, osg_prefix , mod64, share_dir) 
		elif args.machine == 'x32':
			print(f"{tr('Installing OSLG (i386) Version => 2.0-beta-src2 ...')}")
			BIOS(args.root, osg_prefix , mod32, share_dir) 	 
	 
