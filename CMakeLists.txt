cmake_minimum_required(VERSION 3.16)

project(rcut C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_RCUT_SDL "Build rcut_sdl" ON)

find_package(Threads REQUIRED)
if(BUILD_RCUT_SDL)
	find_package(SDL3 REQUIRED)
endif()

find_package(harfbuzz QUIET)
find_path(HB_MINI_DIR hb_mini.h hb_mini.c HINTS ${CMAKE_CURRENT_SOURCE_DIR}/.. PATH_SUFFIXES hb_mini)

if(NOT FONT_BACKEND)
	set(FONT_BACKEND "auto" CACHE STRING "Which font backend to use for rcut. Options are: harfbuzz, hb_mini, stb, auto. Defaults to auto.")
	set_property(CACHE FONT_BACKEND PROPERTY STRINGS harfbuzz hb_mini stb auto)
endif()

if(harfbuzz_FOUND AND NOT FONT_BACKEND MATCHES "hb_mini|stb")
	message(STATUS "Using harfbuzz")
	find_package(PNG REQUIRED)
	set(RESOLVED_FONT_DEFINE RCUT_USE_HARFBUZZ)
	set(FONT_LIBS harfbuzz::harfbuzz harfbuzz::harfbuzz-raster)
elseif(HB_MINI_DIR AND NOT FONT_BACKEND STREQUAL "stb")
	message(STATUS "Using hb_mini: ${HB_MINI_DIR}")
	add_library(hb_mini STATIC ${HB_MINI_DIR}/hb_mini.c)
	target_include_directories(hb_mini PUBLIC ${HB_MINI_DIR})
	set(RESOLVED_FONT_DEFINE RCUT_USE_HB_MINI)
	set(FONT_LIBS hb_mini)
else()
	message(STATUS "Using stb_truetype")
	set(RESOLVED_FONT_DEFINE RCUT_USE_STB_TRUETYPE)
endif()

add_library(rcut_core STATIC core/core.c core/layout.c core/layout_iterator.c core/input.c core/widgets.c core/text.c core/menu.c)
if(NOT WIN32)
	target_link_libraries(rcut_core PUBLIC m)
endif()
target_include_directories(rcut_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(rcut_draw STATIC draw/draw.c draw/text.c draw/raster.c draw/theme.c draw/software.c draw/font_hb.c draw/font_stb.c)
target_link_libraries(rcut_draw PUBLIC rcut_core ${FONT_LIBS})
target_compile_definitions(rcut_draw PUBLIC ${RESOLVED_FONT_DEFINE})

if(BUILD_RCUT_SDL)
	add_library(rcut_sdl STATIC rcut_sdl.c)
	target_link_libraries(rcut_sdl PUBLIC rcut_draw SDL3::SDL3 Threads::Threads)
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES)
	add_executable(gallery example/gallery.c)
	target_link_libraries(gallery rcut_sdl)
	add_executable(styles_example example/styles.c)
	target_link_libraries(styles_example rcut_sdl)
	foreach(step tutorial_1_0_hello tutorial_1_1_binding tutorial_1_2_layout tutorial_1_3_resizable tutorial_1_4_cutting)
		add_executable(${step} example/${step}.c)
		target_link_libraries(${step} rcut_sdl)
	endforeach()
endif()

# docs target: Documentation
add_executable(api_documentation EXCLUDE_FROM_ALL docs/api_documentation.c)
target_link_libraries(api_documentation rcut_sdl)
set(TUTORIAL_SHOTS)
foreach(step tutorial_1_0_hello tutorial_1_1_binding tutorial_1_2_layout tutorial_1_4_cutting)
	add_executable(${step}_shot EXCLUDE_FROM_ALL example/${step}.c)
	target_link_libraries(${step}_shot rcut_sdl)
	target_compile_options(${step}_shot PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/docs/tutorial_shot.h)
	target_compile_definitions(${step}_shot PRIVATE SHOT="${CMAKE_CURRENT_SOURCE_DIR}/docs/api/img/${step}.png")
	list(APPEND TUTORIAL_SHOTS COMMAND ${step}_shot)
endforeach()
find_package(Python3 COMPONENTS Interpreter)
add_custom_target(docs
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs/api/img
	${TUTORIAL_SHOTS}
	COMMAND api_documentation ${CMAKE_CURRENT_SOURCE_DIR}/docs/api/img
	COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/docs/gen_api.py
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS api_documentation tutorial_1_0_hello_shot tutorial_1_1_binding_shot tutorial_1_2_layout_shot tutorial_1_4_cutting_shot)

# A misaligned hot loop in the rasterization can lead to up to 40% performance loss.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	set_source_files_properties(draw/software.c PROPERTIES COMPILE_OPTIONS "-falign-loops=64")
endif()
