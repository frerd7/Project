#! /bin/sh
#
# Copyright (C) 2002,2006,2008  Free Software Foundation, Inc.
#
# This gensymlist.sh is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

### The configure script will replace these variables.

: ${srcdir=.}
: ${CC=gcc}

u=
grep "^#define HAVE_ASM_USCORE" config.h >/dev/null 2>&1 && u="_"

$CC  -Os -Wall -W -Wshadow -Wpointer-arith -Wmissing-prototypes                -Wundef -Wstrict-prototypes -g -falign-jumps=1 -falign-loops=1 -falign-functions=1 -mno-mmx -mno-sse -mno-sse2 -mno-3dnow -fno-dwarf2-cfi-asm -DGRUB_MACHINE_PCBIOS=1 -m32 -fno-PIE -fno-stack-protector -mno-stack-arg-probe -Werror -DGRUB_SYMBOL_GENERATOR=1 -E -I. -Iinclude -I"$srcdir/include" $* \
  | grep -v '^#' \
  | sed -n \
        -e '/EXPORT_FUNC *([a-zA-Z0-9_]*)/{s/.*EXPORT_FUNC *(\([a-zA-Z0-9_]*\)).*/'"$u"'\1 kernel/;p;}' \
        -e '/EXPORT_VAR *([a-zA-Z0-9_]*)/{s/.*EXPORT_VAR *(\([a-zA-Z0-9_]*\)).*/'"$u"'\1 kernel/;p;}' \
  | sort -u
