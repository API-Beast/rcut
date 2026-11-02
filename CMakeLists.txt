cmake_minimum_required(VERSION 3.16)

project(rcut C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(SDL3 REQUIRED)

find_path(HB_MINI_DIR hb_mini.h hb_mini.c HINTS ${CMAKE_CURRENT_SOURCE_DIR}/.. PATH_SUFFIXES hb_mini)
if(HB_MINI_DIR)
	message(STATUS "Using hb_mini: ${HB_MINI_DIR}")
	add_library(hb_mini STATIC ${HB_MINI_DIR}/hb_mini.c)
	target_include_directories(hb_mini PUBLIC ${HB_MINI_DIR})
	set(HARFBUZZ_LIBS hb_mini)
else()
	find_package(PNG REQUIRED) # Required by harfbuzz-raster
	find_package(harfbuzz REQUIRED)
	set(HARFBUZZ_LIBS harfbuzz::harfbuzz harfbuzz::harfbuzz-raster)
endif()

add_library(rcut_core STATIC core/core.c core/layout.c core/layout_iterator.c core/input.c core/widgets.c core/text.c core/menu.c)
if(NOT WIN32)
	target_link_libraries(rcut_core PUBLIC m)
endif()
add_library(rcut_draw STATIC draw/draw.c draw/text.c draw/raster.c draw/theme.c draw/software.c)
target_link_libraries(rcut_draw PUBLIC rcut_core ${HARFBUZZ_LIBS})
add_library(rcut_sdl STATIC sdl/sdl.c)
target_link_libraries(rcut_sdl PUBLIC rcut_draw SDL3::SDL3-static Threads::Threads)
target_include_directories(rcut_sdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(gallery example/gallery.c)
target_link_libraries(gallery rcut_sdl)
add_executable(styles_example example/styles.c)
target_link_libraries(styles_example rcut_sdl)

# A misaligned hot loop in the rasterization can lead to up to 40% performance loss.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	set_source_files_properties(draw/software.c PROPERTIES COMPILE_OPTIONS "-falign-loops=64")
endif()
