set(SOURCES 
    launcher.cpp
    crt.cpp
)

add_executable(launcher ${SOURCES})

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
