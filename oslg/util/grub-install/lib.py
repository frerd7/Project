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
import platform
import shutil
import argparse
import locale
import re 

def check_root():
	if os.geteuid() != 0:
         raise Exception(f"{tr(f'The program can only be used in root mode, use sudo {sys.argv[0]} to continue')}")

def unicode_string(string):
    s = string.strip().decode("utf-8")
    return s

def write_lines(file, args):
	file.write(f"{args}\n")
	
def get_system_language():
    system_locale, _ = locale.getdefaultlocale()
    system_language = system_locale.split('_')[0] if system_locale else 'en'
    return system_language
    
def tr(phrase):
    system_language = get_system_language()
    translations = {
        'es': {f'The program can only be used in root mode, use sudo {sys.argv[0]} to continue': f'El programa solo puede usarse en modo root, usa sudo {sys.argv[0]} para continuar',
               'OSLG Bootloader Installer.' : 'Instalador del gestor de arranque OSLG.', 
               'Install OSLG in a specific directory.' : 'Instala a OSLG en un directorio específico.' ,
               'Module Specific Directory.' : 'Directorio específico de módulos.',
               'Supported machines i386 (x32), x86_64 (x64).' : 'Maquinas soportadas i386 (x32), x86_64 (x64).', 
               'No device directory specified, use -r or --root directory' : 'No se a especificado un directorio de dispositivo, usa -r o --root directorio', 
               'Platform Device not supported' : 'Platforma Dispositivo no soportado', 
               'Installing OSLG (i386) Version => 2.0-beta-src2 ...' : 'Instalación de OSLG (i386) Versión => 2.0-beta-src2 ...' ,
               'Installing OSLG (x86_64) Version => 2.0-beta-src2 ...' : 'Instalación de OSLG (x86_64) Versión => 2.0-beta-src2 ...', 
               'Cannot access modules files directory.' : 'No se puede acceder al directorio de archivos módulos.', 
               'Cant access the modules directory, try reinstalling the .mod files or type -d or --directory path' : 'No se puede acceder al directorio de archivos módulos, prueba reintalando los archivos .mod o escribe -d o --directorio path', 
               'Device partition not supported for system BIOS/UEFI' : 'Partición del dispositivo no soportada para el sistema BIOS/UEFI', 
               'Installation finished. No error reported.' : 'Instalacion finalizada. No se reporto errores.',
			   'Device:' : 'Dispositivo:'},
               
        'en': {f'El programa solo puede usarse en modo root, usa sudo {sys.argv[0]} para continuar ': f'The program can only be used in root mode, use sudo {sys.argv[0]} to continue',
               'Instalador del gestor de arranque OSLG.' : 'OSLG Bootloader Installer.' ,
               'Instala a OSLG en un directorio específico.' : 'Install OSLG in a specific directory.' ,
               'Directorio específico de módulos.' : 'Module Specific Directory.' ,
			   'Maquinas soportadas i386 (x32), x86_64 (x64).' : 'Supported machines i386 (x32), x86_64 (x64).' ,
			   'No se a especificado un directorio de dispositivo, usa -r o --root directorio' : 'No device directory specified, use -r or --root directory', 
			   'Platforma Dispositivo no soportado' : 'Platform Device not supported', 
			   'Instalacion de OSLG (i386) Versión => 2.0-beta-src2 ...' : 'Installing OSLG (i386) Version => 2.0-beta-src2 ...', 
			   'Instalación de OSLG (x86_64) Versión => 2.0-beta-src2 ...' : 'Installing OSLG (x86_64) Version => 2.0-beta-src2 ...', 
			   'No se puede acceder al directorio de archivos módulos.' : 'Cannot access modules files directory.', 
			   'No se puede acceder al directorio de archivos módulos, prueba reintalando los archivos .mod o escribe -d o --directorio path' : 'Cant access the modules directory, try reinstalling the .mod files or type -d or --directory path', 
			   'Partición del dispositivo no soportada para el sistema BIOS/UEFI' : 'Device partition not supported for system BIOS/UEFI', 
			   'Instalacion finalizada. No se reporto errores.' : 'Installation finished. No error reported.',
			   'Dispositivo:' : 'Device:'},
			   
    }
    translated_phrase = translations.get(system_language, {}).get(phrase, phrase)
    return translated_phrase
	
class DeviceMap():
	""
	"Map Partition Filesystem"
	"Check Device"
	""
	def __init__(self, root, prefix, mkdevice):
		self.devicemap = f"{root}{prefix}/device.map"
		subprocess.run(f"{mkdevice} -m {self.devicemap}", shell=True)
	
class Probe(): 
	""
	"Check Device Path"
	"Information to fs, fs_uuid, device name"
	""
	def __init__(self, root, probe):
		self.root = root
		self.probe = probe
		
	def device(self, options):
		if options == 'type':
			type = subprocess.check_output(f"{self.probe} --target=fs {self.root}", shell=True)
			fs_t = unicode_string(type)
			return fs_t
		elif options == 'device':
			device	= subprocess.check_output(f"{self.probe} --target=device {self.root}", shell=True)
			dev = unicode_string(device)
			return dev
		elif options == 'uuid':
			fs_uuid	= subprocess.check_output(f"{self.probe} --target=fs_uuid {self.root}", shell=True)
			uuid = unicode_string(fs_uuid)
			return uuid

class Add_Modules:
	"copy modules to dir entry"
	""
	def conf_theme(self):
		if os.path.exists(self.locale) == False:
			os.mkdir(self.locale)
		else:
			shutil.rmtree(self.locale)
			os.mkdir(self.locale)

		if os.path.exists(self.copy_thems) == True or os.path.exists(self.copy_fonts) == True:
			if os.path.exists(self.theme) == False or os.path.exists(self.fonts) == False:
				shutil.copytree(self.copy_thems, self.theme)
				shutil.copytree(self.copy_fonts, self.fonts)
			else:
				shutil.rmtree(self.theme)
				shutil.rmtree(self.fonts)
				shutil.copytree(self.copy_thems, self.theme)
				shutil.copytree(self.copy_fonts, self.fonts) 

		subprocess.run(f"{self.edit}/oslgenv create", shell=True) 
               
	def __init__(self, mod, prefix, share_dir, edit):
		self.locale = f"{prefix}/locale"
		self.theme = f"{prefix}/themes"
		self.fonts = f"{prefix}/fonts"
		self.copy_thems = f"{share_dir}/themes"
		self.copy_fonts = f"{share_dir}/fonts"
		self.edit = f"{edit}{prefix}"
		  
		for files in os.listdir(mod):
			if files.endswith('.mod') or files.endswith('.o') or files.endswith('.lst') or files.endswith('.img'):
				name = os.path.join(mod, files)
				shutil.copy(name, prefix)
				   
		self.conf_theme()	

class init_cfg():
	""
	"Create Init preload modules"
	""
	def __init__(self, mod, prefix, video):
		mod_init = ["video", "video_fb"
		    , "efi_gop", "pci"
		    , "efi_uga", "vbe"
		    , "nmenu", "gfxterm"
		    , "font", "bitmap"
		    , "gfxmenu", "gfxrgn"
		    ]
		    
		print("Generated init preload mod ...")
		init  = open (f"{prefix}/init.cfg", "w+")
		for line in mod_init:
			if os.path.exists(f"{mod}/{line}.mod") == True:
				if line == 'video':
					write_lines(init, f"{line}: viedo_fb")
				if video == 1:
					if line == 'efi_gop':
						write_lines(init, f"{line}: pci")
					if line == 'efi_uga':
						write_lines(init, f"{line}: pci")
				if line == 'nmenu':
					write_lines(init, f"{line}: gfxterm")
				elif line == 'gfxterm':
					write_lines(init, f"{line}: bitmap")
				elif line == 'gfxmenu':
					write_lines(init, f"{line}: font")
				elif line == 'gfxrgn':
					init.write(f"{line}: font")
		init.close()


def determine_device_dev(root, probe):
	device_mount = Probe(root, probe)
	order = r'[0-9]'
	char_str = re.sub(order, '', device_mount.device('device'))
	if char_str == '/dev/loop':
		dev = " " + device_mount.device('device')
		return dev
	else:
		dev = " " + char_str
		return dev