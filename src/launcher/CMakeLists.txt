include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)

FetchContent_MakeAvailable(imgui)

set(SOURCES 
    os.cpp
    texture.cpp
    renderpass.cpp
    framebuffer.cpp
    shader.cpp
    launcher.cpp
    crt.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp    
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

add_executable(launcher ${SOURCES})

target_include_directories(
    launcher
    PUBLIC
    shader
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(launcher PRIVATE
    $<$<CONFIG:Debug>:DEBUG="${CMAKE_CURRENT_SOURCE_DIR}">
    $<$<CONFIG:Debug>:DEBUG_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}">
)

# 1. Platform-Specific System Library Configuration
if(UNIX AND NOT APPLE)
    # Linux requires the 'dl' library for dlopen/dlsym architecture
    target_link_libraries(launcher PRIVATE dl)
    
    # Modern Linux systems find OpenGL via local system targets
    find_package(OpenGL REQUIRED)
    target_link_libraries(launcher PRIVATE OpenGL::GL)

elseif(WIN32)
    # Windows native API handles runtime dynamic loading without external libraries
    # Link native OpenGL subsystem
    target_link_libraries(launcher PRIVATE opengl32)
    
    # Prevent Windows.h from injecting messy, conflicting macros
    target_compile_definitions(launcher PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# 2. Cross-Platform SDL2 Dependency Resolution
# This leverages standard system packaging setups
find_package(SDL2 REQUIRED)

if(TARGET SDL2::SDL2)
    # Modern CMake standard target mapping
    target_link_libraries(launcher PRIVATE SDL2::SDL2)
elseif(SDL2_FOUND)
    # Legacy fallback mapping
    target_include_directories(launcher PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(launcher PRIVATE ${SDL2_LIBRARIES})
endif()

# Copy the shaders folder to the output directory at build time
# This ensures that whenever you build, the shaders sit exactly next to the binary.
add_custom_command(
    TARGET launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:launcher>/shaders"
    COMMENT "Copying runtime shaders to build directory..."
)