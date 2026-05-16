cmake_minimum_required(VERSION 3.10)
project(my_core_libretro LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(my_core_libretro MODULE
    src/framework/common.cpp
    src/framework/types.cpp
    src/framework/sprites.cpp
    src/framework/palette.cpp
    src/framework/vdp_base.cpp
    src/framework/vdp_base.cpp
    src/framework/vdp_utils.cpp
    src/framework/vdp_nes.cpp
    src/core.cpp
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
