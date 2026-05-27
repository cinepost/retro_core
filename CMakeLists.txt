cmake_minimum_required(VERSION 3.10)
project(my_core_libretro LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define file paths for static data generation
set(NES_DEFAULT_FONT_CHR_INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/assets/nes/nes_default_font.chr")
set(NES_DEFAULT_FONT_CHR_GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/sprites_chr.cpp")
set(NES_CHR_CONVERTER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/nes_chr_to_cpp.py")

# Find Python interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}"
    COMMAND Python3::Interpreter "${NES_CHR_CONVERTER_SCRIPT}" "${NES_DEFAULT_FONT_CHR_INPUT_FILE}" "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}" "DefaultFontCHR"
    DEPENDS "${NES_DEFAULT_FONT_CHR_INPUT_FILE}" "${NES_CHR_CONVERTER_SCRIPT}"
    COMMENT "Converting NES default font CHR data to static C++ array"
    VERBATIM
)

add_library(my_core_libretro MODULE
    external/midifile/src/MidiFile.cpp
    external/midifile/src/MidiMessage.cpp
    external/midifile/src/MidiEvent.cpp
    external/midifile/src/MidiEventList.cpp
    src/framework/common.cpp
    src/framework/types.cpp
    src/framework/sprites.cpp
    src/framework/palette.cpp
    src/framework/renderer.cpp
    src/framework/vdp_utils.cpp
    src/framework/sound.cpp
    src/framework/ppu/ppu_nes.cpp
    src/framework/ppu/ppu_msx.cpp
    src/framework/apu/apu_nes.cpp
    src/framework/apu/apu_psg.cpp
    src/framework/apu/apu_ym2612.cpp
    src/core.cpp

    #Generated static data
    "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}"
)

set_target_properties(my_core_libretro PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX "" 
    OUTPUT_NAME "retro_core"
)

target_include_directories(my_core_libretro PRIVATE 
	/usr/include/libretro-common
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Target the current machine's hardware profile natively
add_compile_options(-march=native)

target_include_directories(my_core_libretro PRIVATE external/midifile/include)

if(UNIX AND NOT APPLE)
    target_link_libraries(my_core_libretro PRIVATE m dl)
endif()
