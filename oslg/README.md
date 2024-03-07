# OSLG (Operating System Loader in Grub) (C)

It is a boot manager based on the old burg project and uses GNU GRUB technology.

This version of grub aims to replace the bootloader style terminal and combine the graphics with letters to give some style to the startup.

## Installation
	git clone git@github.com:frerd7/Project.git
	cd ./Proyect/oslg
	make && sudo make install

        to try download qemu:
	sudo apt-get install qemu-system-i386

	Create file img:
	qemu-img create ./boot.img 5M && mkfs.ext2 -F -m0 ./boot.img

	Install:
	sudo mount -t ext2 -o loop ./boot.img /mnt
        sudo oslg-install --root /mnt && umount /mnt

	Run:
	qemu-system-i386 -hda ./boot.img -m 400

        [Warning]:
	Do not install on a system partition since this is beta
