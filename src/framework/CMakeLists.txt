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

add_library(retro_core_lib STATIC
    ${EXTERNAL_DIR}/lodepng/lodepng.cpp
    ${EXTERNAL_DIR}/midifile/src/MidiFile.cpp
    ${EXTERNAL_DIR}/midifile/src/MidiMessage.cpp
    ${EXTERNAL_DIR}/midifile/src/MidiEvent.cpp
    ${EXTERNAL_DIR}/midifile/src/MidiEventList.cpp
    
    common.cpp
    types.cpp
    sprites.cpp
    palette.cpp
    renderer.cpp
    vdp_utils.cpp
    sound.cpp
    ppu/ppu_nes.cpp
    ppu/ppu_msx.cpp
    ppu/ppu_msx_utils.cpp
    ppu/ppu_utils.cpp
    apu/apu_nes.cpp
    apu/apu_psg.cpp
    apu/apu_ym2612.cpp
    game_engine/engine_core.cpp

#    #Generated static data
#    "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}"

#    #Static data
#    assets/msx/knightmare_ref.cpp
)

add_compile_options(-march=native)

set_target_properties(retro_core_lib PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX "" 
    OUTPUT_NAME "retro_core"
)

target_include_directories(retro_core_lib PRIVATE 
	/usr/include/libretro-common
	${PROJECT_DIR}/src
)

# Target the current machine's hardware profile natively
target_compile_options(retro_core_lib PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    
    # Debug-specific flags: Add maximum GDB symbols (-ggdb3) and disable optimizations
    $<$<CONFIG:Debug>:-ggdb3 -O0>
    
    # Release-specific flags: High optimization and vectorization
    $<$<CONFIG:Release>:-O3 -march=native>
    
    # Release with Debug Info flags: Optimization with standard debug symbols
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
)

target_include_directories(retro_core_lib PRIVATE ${EXTERNAL_DIR})
target_include_directories(retro_core_lib PRIVATE ${EXTERNAL_DIR}/midifile/include)

target_compile_definitions(retro_core_lib PRIVATE 
    SOL_ALL_SAFETIES_ON=1
    $<$<CONFIG:Debug>:DEBUG_MODE=1>
    $<$<CONFIG:Release>:RELEASE_MODE=1>
)
#target_link_libraries(retro_core_lib PRIVATE luajit sol2)

if(UNIX AND NOT APPLE)
    target_link_libraries(retro_core_lib PRIVATE m dl)

    # Re-export symbols so C-extensions loaded by Lua can see engine APIs
    target_link_options(retro_core_lib PRIVATE "-Wl,-E")
endif()

