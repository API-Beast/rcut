# rcut

Provisional README, very technical.

## Modules

**rcut_core** handles all the layouting, input handling and widget logic in rcut. It has no dependencies except for libc and as a result can be embedded in any project and any platform. It can be used standalone, which makes sense if you don't want to use any of the theming rcut_draw provides. (TODO: Documentation for how to write a rcut_core frontend.)

**rcut_draw** is based upon rcut_core and adds a software renderer as well as a themeing API. It depends only on rcut_core and stb_truetype (or alternatively Harfbuzz, see [Text backend](#text-backend)). To use rcut_draw in your own 3D renderer all you need is a custom caching/drawing backend. To embed it in a software renderer, all you need to do is provide it with a ARGB buffer to draw into, using r_draw_software_set_target. (TODO: Documentation for how to write a custom texture cache.)

**rcut_sdl** is a default frontend for rcut_core and rcut_draw, depending on, you guessed it, SDL 3. It is much smaller than the other modules because it is essentially just platform glue code. You can use it's code as a reference for how to implement a rcut_core frontend.

## Amalgamation

```sh
./amalgamate.sh   # or amalgamate.bat
```

This concats all the source files needed for rcut_core and rcut_draw in a single .c file each. This allows you to use rcut without the hassle of CMake and git.

To use, simple copy the `rcut_*.{c/h}` files alongside stb_truetype.h into your project folder and add `rcut_*.c` to your compilation.

## CMake subproject

If you don't want to use the Amalgamation, then using rcut as a CMake subdirectory is recommended.

First add rcut as a git submodule:
```sh
git submodule add https://github.com/API-Beast/rcut.git rcut # Add to existing project
git submodule update --init      # after cloning your project
git submodule update --remote    # update rcut to latest
```

Then add it as a subproject to your CMakeLists.txt:
```cmake
add_subdirectory(rcut)
target_link_libraries(app rcut_sdl)
```

| Option                | Default     |                                                        |
|-----------------------|-------------|--------------------------------------------------------|
| `BUILD_EXAMPLES` | `OFF`       | Build `gallery`, `styles_example` and the `tutorial_*` steps. |
| `FONT_BACKEND`   | auto        | `hb_mini` or `stb` to skip harfbuzz/hb_mini detection, see [Text backend](#text-backend). |
| `HB_MINI_DIR`         | `../hb_mini` | Directory containing `hb_mini.c`/`hb_mini.h`.          |

As a subproject, set these before `add_subdirectory`:

```cmake
set(FONT_BACKEND stb)
add_subdirectory(rcut)
unset(FONT_BACKEND) # only necessary if you have your own FONT_BACKEND cached option.
```

Targets: `rcut_core`, `rcut_draw`, `rcut_sdl` (static libraries), `docs` (regenerates `docs/api`, needs Python 3 and markdown python package).

## Documentation

```sh
cmake -B build && cmake --build build --target docs
```

Writes HTML to `docs/api/`. Needs Python 3 with `markdown`; `rcut.chm` is built too if `chmcmd` (Free Pascal) is in PATH.

## Text backend

| Define                  | Dependency                 | Shaping |
|-------------------------|----------------------------|---------|
| `RCUT_USE_STB_TRUETYPE` | `stb_truetype.h` (bundled) | None, 1 codepoint = 1 glyph, `kern` table only. No color glyphs. |
| `RCUT_USE_HB_MINI`      | [hb_mini](https://github.com/API-Beast/hb_mini) | GSUB/GPOS, COLR/PNG glyphs. No USE, Khmer/Myanmar, vertical, legacy `kern`, AAT, CFF2. |
| `RCUT_USE_HARFBUZZ`     | harfbuzz, harfbuzz-raster  | Full. |

hb_mini is a C subset of HarfBuzz with the same API, so it can be swapped for HarfBuzz without code changes.

Amalgamation: define one of the above, or include `hb_mini.h`/`hb.h` before `rcut_draw.h`. Defaults to stb_truetype.

CMake: harfbuzz if found, else hb_mini from `../hb_mini/`, else stb_truetype. Override with `-DRCUT_FONT_BACKEND=hb_mini|stb`.
