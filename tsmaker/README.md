# TSMAKER (C/C++)

It is a program that is used to carry out compilation tasks, its objective is to make the compilation faster.

## ¿How does it work?
	[Steps to follow]
	    Step 1: Create a deps file in the build directory.
	     Example:
		  hello.c file
			#include <stdio.h>
			int main() {
				printf("Hello Word!");
				return 0;
			}
		   deps file
			%gcc: hello.c -o hello

	    Setp 2: Open the terminal and type [tsmaker] in the project directory
	[Form 2]
	  You can use a Makefile, autogen.sh, configure, CMakeLists.txt	

## Usage
	Variables used in the deps:
		%url: file download address [valid: *https://addr/file.zip]
		%install: install packages [example: bison]
		%gcc: comand gcc [example: test.c -o test]
		%gxx: comand gxx [example: test.cpp -o test]
		%prefix: Prefix used install [example: ./]
		%cmake: options in comand [example: -DCMAKE_PREFIX_INSTALL]
		%make: options in comand [example: -j2]
		%configure: options in command [example: --with-platform=efi]

## Installation
	git clone https://github.com/frerd7/Project.git
	cd ./Project/tsmaker
	make && make install
