@echo off
cd /d "%~dp0"
copy /y draw\stb_truetype.h . >nul

> rcut_core.c (
	echo #include "rcut_core.h"
	findstr /v /c:"#include \"internal.h\"" /c:"#include \"../" /c:"#define _DEFAULT_SOURCE" core\internal.h
	for %%f in (core\*.c) do findstr /v /c:"#include \"internal.h\"" /c:"#include \"../" /c:"#define _DEFAULT_SOURCE" %%f
)

> rcut_draw.c (
	echo #define _DEFAULT_SOURCE
	echo #include "rcut_draw.h"
	findstr /v /c:"#include \"internal.h\"" /c:"#include \"../" /c:"#define _DEFAULT_SOURCE" core\internal.h
	findstr /v /c:"#include \"internal.h\"" /c:"#include \"../" /c:"#define _DEFAULT_SOURCE" draw\internal.h
	for %%f in (draw\*.c) do findstr /v /c:"#include \"internal.h\"" /c:"#include \"../" /c:"#define _DEFAULT_SOURCE" %%f
)

> rcut_sdl.c (
	echo #include "rcut_sdl.h"
	for %%f in (sdl\*.c) do findstr /v /c:"#include \"internal.h\"" /c:"#include \"../" /c:"#define _DEFAULT_SOURCE" %%f
)
