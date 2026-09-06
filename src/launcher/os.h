#ifndef __RETRO_CORE_LAUNCHER_OS_H
#define __RETRO_CORE_LAUNCHER_OS_H

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

#if !defined(DEBUG_SOURCE_DIR)
    // Only include heavy OS headers if we are NOT in Debug mode
    #if defined(_WIN32)
    #include <windows.h>
    #elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
    #elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #endif
#endif

// Returns the absolute directory path where the binary executable resides
fs::path getExecutableDir();

// Reads text file
std::string readTextFile(const std::string& filePath);

#endif  // __RETRO_CORE_LAUNCHER_OS_H