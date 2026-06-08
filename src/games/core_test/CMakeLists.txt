# Define file paths for static data generation
set(NES_DEFAULT_FONT_CHR_INPUT_FILE "${PROJECT_DIR}/assets/nes/nes_default_font.chr")
set(NES_DEFAULT_FONT_CHR_GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/sprites_chr.cpp")
set(NES_CHR_CONVERTER_SCRIPT "${PROJECT_DIR}/scripts/nes_chr_to_cpp.py")

# Find Python interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}"
    COMMAND Python3::Interpreter "${NES_CHR_CONVERTER_SCRIPT}" "${NES_DEFAULT_FONT_CHR_INPUT_FILE}" "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}" "DefaultFontCHR"
    DEPENDS "${NES_DEFAULT_FONT_CHR_INPUT_FILE}" "${NES_CHR_CONVERTER_SCRIPT}"
    COMMENT "Converting NES default font CHR data to static C++ array"
    VERBATIM
)

add_compile_options(-march=native)

add_library(core_test MODULE
    core.cpp
    game.cpp
    boot_state.cpp
    intro_state.cpp
    level1_state.cpp

    #Generated static data
    "${NES_DEFAULT_FONT_CHR_GENERATED_CPP}"

    #Static data
    ${PROJECT_DIR}/assets/msx/knightmare_ref.cpp
)

target_compile_options(core_test PRIVATE
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

set_target_properties(core_test PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX "" 
    OUTPUT_NAME "core_test"
)

target_link_libraries(core_test
    PUBLIC
    retro_core_lib
)

target_include_directories(core_test PRIVATE 
	/usr/include/libretro-common
	${PROJECT_DIR}/src
    ${EXTERNAL_DIR}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(core_test PRIVATE m dl)

    # Re-export symbols so C-extensions loaded by Lua can see engine APIs
    target_link_options(core_test PRIVATE "-Wl,-E")
endif()

