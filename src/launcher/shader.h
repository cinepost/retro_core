#ifndef __RETRO_CORE_LAUNCHER_SHADER_H
#define __RETRO_CORE_LAUNCHER_SHADER_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <cstring>

#include "shaderparam.h"

namespace fs = std::filesystem;

namespace RetroLauncher {


class Shader {
    public:
        using ParamType = ShaderParameter::ParamType;
        using DefinesList = std::unordered_map<std::string, std::string>;

        Shader(const std::string& name);
        Shader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
        Shader(const std::string& name, const std::string& vertPath, const std::string& fragPath, const std::string& geomPath);
        ~Shader();

        bool init(const std::string& vertPath, const std::string& fragPath);
        bool init(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath);
        bool checkAndReload(bool force = false);
        void destroy();
        void drawUI();

        void resetShaderParameters();

        template<typename T>
        T getShaderParameterValue(const std::string& parmName, const T& defaultValue) const;

        void linkShaderParameter(const std::string& parmName, const uint32_t& source, uint32_t min_value, uint32_t max_value);

        const std::string& getName() const { return mName; }

    public:
        // GL side API
        GLint getProgram() const { return mProgramID; }

        void use() {
            assert(mInitialized);

            if(mNeedRecompile) {
                checkAndReload(true);
            }

            assert(mProgramID);
            glUseProgram(mProgramID);

            setShaderParameters();
        }

        GLint getUniformLocation(const std::string& name) const {
            // If the location is already cached, return it instantly
            if (mUniformLocationCache.find(name) != mUniformLocationCache.end()) {
                return mUniformLocationCache[name];
            }

            // Otherwise, ask OpenGL for the location using the CURRENT live programID
            int location = glGetUniformLocation(mProgramID, name.c_str());

            #ifdef DEBUG            
            // -1 means the uniform doesn't exist or was optimized away by the GLSL compiler
            if (location == -1) {
                // Optional: Print a warning during development, but don't crash
                std::cerr << "[Shader] Warning: \"" << mName << "\" uniform '" << name << "' not found/active." << std::endl;
            }
            #endif // DEBUG

            mUniformLocationCache[name] = location;
            return location;
        }

        // --- Standard Sampler Setup ---
        void setTextureSlot(const std::string& name, int slotIndex) const {
            // Example: setTextureSlot("u_DiffuseMap", 0);
            // This tells the shader sampler to listen to GL_TEXTURE0
            setInt(name, slotIndex);
        }

        // --- Active Push Texture Wrapper (Call This Inside Your Render Loop) ---
        // This looks up the uniform, tells the shader which slot to read from, 
        // and binds the actual GPU texture handle to that slot instantly.
        void pushTexture(const std::string& name, unsigned int textureID, int slotIndex) const {
            // 1. Tell the shader sampler which texture unit slot it belongs to (e.g. 0, 1, 2)
            setInt(name, slotIndex);

            // 2. Activate the corresponding hardware texture unit
            // GL_TEXTURE0 is defined as an incremental integer offset, so GL_TEXTURE0 + slotIndex works perfectly
            glActiveTexture(GL_TEXTURE0 + slotIndex);

            // 3. Bind your actual GPU texture object ID to the active unit
            glBindTexture(GL_TEXTURE_2D, textureID);
        }

        // --- Uniform Setters ---
        void setBool(const std::string& name, bool value) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform1i(location, static_cast<int>(value));
        }

        void setInt(const std::string& name, int32_t value) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform1i(location, value);
        }

        void setInt2(const std::string& name, int32_t x, int32_t y) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform2i(location, x, y);
        }

        void setInt3(const std::string& name, int32_t x, int32_t y, int32_t z) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform3i(location, x, y, z);
        }

        void setInt4(const std::string& name, int32_t x, int32_t y, int32_t z, int32_t w) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform4i(location, x, y, z, w);
        }
        
        void setUint(const std::string& name, uint32_t value) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform1ui(location, value);
        }

        void setUint2(const std::string& name, uint32_t x, uint32_t y) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform2ui(location, x, y);
        }

        void setUint3(const std::string& name, uint32_t x, uint32_t y, uint32_t z) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform3ui(location, x, y, z);
        }

        void setUint4(const std::string& name, uint32_t x, uint32_t y, uint32_t z, uint32_t w) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform4ui(location, x, y, z, w);
        }

        void setFloat(const std::string& name, float value) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform1f(location, value);
        }

        void setVec2(const std::string& name, float x, float y) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform2f(location, x, y);
        }

        void setVec3(const std::string& name, float x, float y, float z) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform3f(location, x, y, z);
        }

        void setVec4(const std::string& name, float x, float y, float z, float w) const {
            GLint location = getUniformLocation(name);
            if(location == -1) return;
            glUniform4f(location, x, y, z, w);
        }

        void addDefines(const DefinesList& defines) {
            for(const auto& entry: defines) {
                addDefine(entry.first, entry.second);
            }
        }

        void addDefine(const std::string& key, const std::string& value ) {
            auto it = mDefines.find(key);
            if(it != mDefines.end()) {
                if(it->second == value) {
                    return;
                } 
            }

            mDefines[key] = value;
            mNeedRecompile = true;
        }

    private:
        void compileAndLink();
        std::string loadShaderSourceWithIncludes(const std::string& providedPath, int depth = 0);

        static void injectDefines(std::string& source, const DefinesList& defines, const std::string& shaderPath);

        ShaderParameter::ParamType stringToType(const std::string& str, int& outChannels) const;
        void parseParameters(const std::string& fullSourceCode);

        void setShaderParameters() const;

        void clearDefines() {
            mDefines.clear();
        }

        void clearUniformCache() {
            mUniformLocationCache.clear();
        }

        void clearShaderParameters();

    private:
        std::string mName;
        std::string mVertSourcePath;
        std::string mFragSourcePath;
        std::string mGeomSourcePath;

        mutable fs::file_time_type mLastVertShaderSourceWriteTime;
        mutable fs::file_time_type mLastFragShaderSourceWriteTime;
        mutable fs::file_time_type mLastGeomShaderSourceWriteTime;

        mutable std::chrono::steady_clock::time_point mLastCheckTime;

        GLint mProgramID = 0;

        bool mInitialized = false;
        mutable bool mNeedRecompile = true;

    private:
        mutable std::unordered_map<std::string, int> mUniformLocationCache;
        mutable DefinesList mDefines;

        std::unordered_map<std::string, ShaderParameter> mShaderParameters;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_SHADER_H