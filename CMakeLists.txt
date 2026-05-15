cmake_minimum_required(VERSION 3.10)
project(my_core_libretro LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(my_core_libretro MODULE
    src/common.cpp
    src/core.cpp
    src/types.cpp
    src/sprites.cpp
    src/palette.cpp
    src/renderer.cpp
    src/renderer_utils.cpp
    src/renderer_nes.cpp
)

set_target_properties(my_core_libretro PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX "" 
    OUTPUT_NAME "retro_core"
)

target_include_directories(my_core_libretro PRIVATE 
	/usr/include/libretro-common
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(UNIX AND NOT APPLE)
    target_link_libraries(my_core_libretro PRIVATE m dl)
endif()
