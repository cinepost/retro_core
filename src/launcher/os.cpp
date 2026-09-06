#include <fstream>

#include "os.h"

fs::path getExecutableDir() {
#ifdef DEBUG_SOURCE_DIR
    std::cout << "DEBUG_SOURCE_DIR " << std::string(DEBUG_SOURCE_DIR) << std::endl;
    return fs::path(DEBUG_SOURCE_DIR);
#else
    // Fallback using platform APIs if argv tracking isn't ideal
    #if defined(_WIN32)
        wchar_t path[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return fs::path(path).parent_path();
    #elif defined(__linux__)
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        return fs::path(std::string(result, (count > 0) ? count : 0)).parent_path();
    #elif defined(__APPLE__)
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
            return fs::path(path).parent_path();
        return fs::current_path(); // Worst case fallback
    #else
        return fs::current_path(); 
    #endif
#endif
}

std::string readTextFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error could not open: " << filePath << std::endl;
        return "";
    }
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}
