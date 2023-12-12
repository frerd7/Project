TARGET_CC=gcc
TARGET_CFLAGS=-Os -Wall -W -Wshadow -Wpointer-arith -Wmissing-prototypes                -Wundef -Wstrict-prototypes -g -falign-jumps=1 -falign-loops=1 -falign-functions=1 -mno-mmx -mno-sse -mno-sse2 -mno-3dnow -fno-dwarf2-cfi-asm -DGRUB_MACHINE_PCBIOS=1 -m32 -fno-PIE -fno-stack-protector -mno-stack-arg-probe -Werror
TARGET_ASFLAGS=-DGRUB_MACHINE_PCBIOS=1 -m32
TARGET_CPPFLAGS=-nostdinc -isystem /usr/lib/gcc/i686-linux-gnu/7/include -I./include -I. -I./include -Wall -W -I/usr/local/lib/oslg/i386-pc -I/usr/local/include
STRIP=
OBJCONV=@OBJCONV@
TARGET_MODULE_FORMAT=@TARGET_MODULE_FORMAT@
TARGET_APPLE_CC=0
COMMON_ASFLAGS=-nostdinc -fno-builtin -m32
COMMON_CFLAGS=-fno-builtin -mrtd -mregparm=3 -m32
COMMON_LDFLAGS=-m32 -nostdlib
