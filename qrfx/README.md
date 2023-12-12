# QRFX (Recovery Mode) based on Qt5

It is a recovery mode that is intended to replace the default recovery mode, giving it a graphical appearance.

This one works with the X screen server.

## ¿How does it work?
	When starting boot manager, recovery mode is selected.
	Then the kernel is initialized, mounts the ram disk and runs systemd which in turn loads into recovery mode.

## Usage
	Options interface:
	-[Next Normal Boot] Returns to normal system startup.
	-[Exec Bash Script] Load additional scripts.
	-[Fix Bootloader] Fix boot manager.
	-[Software Update] Check for updates using apt.
	-[Run dpkg] Open the package manager.
	-[Clean System] Clean the system.

## Installation
	git --clone git@github.com:frerd7/Project.git
	cd ./Proyectos/qrfx
	make && sudo make install 
	

