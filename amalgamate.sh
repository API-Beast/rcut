#!/bin/sh
cd "$(dirname "$0")" || exit 1
cp draw/stb_truetype.h .
src() { for f in "$@"; do grep -v -F -e '#include "internal.h"' -e '#include "../' -e '#define _DEFAULT_SOURCE' "$f"; done; }

{
	echo '#include "rcut_core.h"'
	src core/internal.h core/*.c
} > rcut_core.c

{
	echo '#define _DEFAULT_SOURCE'
	echo '#include "rcut_draw.h"'
	src core/internal.h draw/internal.h draw/*.c
} > rcut_draw.c

{
	echo '#include "rcut_sdl.h"'
	src sdl/*.c
} > rcut_sdl.c
