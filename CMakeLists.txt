cmake_minimum_required(VERSION 3.14)
project(my_core_libretro LANGUAGES CXX C)

# -----------------------------------------------------------------------------
# ENFORCE BUILD TYPE FOR SINGLE-CONFIG GENERATORS (Make / Ninja)
# -----------------------------------------------------------------------------
set(ALLOWED_BUILD_TYPES "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type specified. Defaulting to 'Debug'.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${ALLOWED_BUILD_TYPES})
else()
    message(STATUS "Build type is: " ${CMAKE_BUILD_TYPE})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

message(STATUS "Using project dir: " ${PROJECT_DIR})
message(STATUS "Using external dir: " ${EXTERNAL_DIR})

add_compile_options(-march=native)

include(FetchContent)

# -------------------------------------------------------------------
# Fetch and Build LuaJIT via a CMake-compatible mirror
# -------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Explicit flags used by various popular LuaJIT CMake wrappers:
set(LUA_TARGET_SHARED OFF CACHE BOOL "Build shared library" FORCE)
set(BUILD_SHARED ON CACHE BOOL "Build shared" FORCE) # Some wrappers use this inversion
set(LUAJIT_BUILD_ALAMG ON CACHE BOOL "Enable whole-program amalgam build" FORCE)


FetchContent_Declare(
    luajit
    GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
    GIT_TAG        v2.1
)

FetchContent_MakeAvailable(luajit)

if(TARGET libluajit)
    set_target_properties(libluajit PROPERTIES TYPE STATIC_LIBRARY)
endif()


# -------------------------------------------------------------------
# Fetch Sol2 (C++ binding library)
# -------------------------------------------------------------------
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.3.0
)
FetchContent_MakeAvailable(sol2)

add_subdirectory( src )

