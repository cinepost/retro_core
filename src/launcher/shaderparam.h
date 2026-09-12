#ifndef __RETRO_CORE_LAUNCHER_SHADERPARAM_H
#define __RETRO_CORE_LAUNCHER_SHADERPARAM_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <cassert>
#include <string>
#include <unordered_map>
#include <cstring>
#include <type_traits>
#include <variant>
#include <vector>
#include <mutex>

namespace RetroLauncher {

class ShaderParameter {
    public:
        ShaderParameter(): mpLinkedData(nullptr), mLinkedElementCount(0), mLinked(false), mType(ParamType::Unknown) {

        }

        enum class ParamType {
            Float, Vec2,  Vec3,  Vec4,
            Int,   Int2,  Int3,  Int4,
            Uint,  Uint2, Uint3, Uint4,
            Bool, Unknown
        };

    public:
        void resetToDefault() {
            if(mLinked || hasExternalSource()) return;

            std::memcpy(mValue, mDefaultValue, sizeof(mDefaultValue));
        }

        bool isLinked() const { return mLinked; }
        void toggleLinkState() { mLinked = !mLinked; }

        const void* getValuePtr() const {
            if(!mLinked || !hasExternalSource()) {
                return &mValue;
            } else {
                assert(mLinkedElementCount > 0);
                fillSourcedValues();
                return &mSourceValue;
            }
        }

        bool hasExternalSource() const {
            return mpLinkedData != nullptr;
        }

        uint32_t parmTypeElementsCount() const {
            return parmTypeElementsCount(mType);
        }

        [[nodiscard]] static constexpr uint32_t parmTypeElementsCount(ParamType pType) noexcept {
            switch(pType) {
                case ParamType::Float:
                case ParamType::Uint:
                case ParamType::Bool:
                case ParamType::Int:
                    return 1;
                    break;
                case ParamType::Vec2:
                case ParamType::Int2:
                case ParamType::Uint2:
                    return 2;
                    break;
                case ParamType::Vec3:
                case ParamType::Int3:
                case ParamType::Uint3:
                    return 3;
                    break;
                case ParamType::Vec4:
                case ParamType::Int4:
                case ParamType::Uint4:
                    return 4;
                    break;
            }
            return 0;
        }

    private:
        void fillSourcedValues() const {
            if(!hasExternalSource()) return;

            std::lock_guard<std::mutex> lock(mMutex);

            #pragma unroll
            for(size_t i = 0; i < mLinkedElementCount; ++i) {
                switch(mType) {
                    case ParamType::Float:
                    case ParamType::Vec2:
                    case ParamType::Vec3:
                    case ParamType::Vec4:
                        mSourceValue[i] = *(reinterpret_cast<const float*>(mpLinkedData) + i);
                    case ParamType::Uint:
                    case ParamType::Uint2:
                    case ParamType::Uint3:
                    case ParamType::Uint4:
                        mSourceValue[i] = static_cast<float>(*(reinterpret_cast<const uint32_t*>(mpLinkedData) + i));
                        break;
                    case ParamType::Int:
                    case ParamType::Int2:
                    case ParamType::Int3:
                    case ParamType::Int4:
                        mSourceValue[i] = static_cast<float>(*(reinterpret_cast<const int32_t*>(mpLinkedData) + i));
                    case ParamType::Bool:
                        mSourceValue[i] = static_cast<float>(*(reinterpret_cast<const bool*>(mpLinkedData) + i));
                        break;
                    default:
                        std::cerr << "[ShaderParameter] unsuppoted type!\n";
                        break;
                }
            }
        }

    private:
        const void* mpLinkedData = nullptr;
        int  mLinkedElementCount = 0;
        bool mLinked; // If true parameter gets value through mpLinkedData (if set)

        std::string mIdentifier;  // GLSL uniform name
        std::string mLabel;       // ImGui title
        ParamType   mType;        // Data type enum

        // Arrays representing up to 4 channels (XYZW) for vectors or single values at index 0
        float mDefaultValue[4]  = {0.0f, 0.0f, 0.0f, 0.0f};
        float mValue[4]         = {0.0f, 0.0f, 0.0f, 0.0f};
        float mMin[4]           = {0.0f, 0.0f, 0.0f, 0.0f};
        float mMax[4]           = {0.0f, 0.0f, 0.0f, 0.0f};


        mutable float mSourceValue[4]   = {0.0f, 0.0f, 0.0f, 0.0f}; // temp source converted value

        mutable std::mutex  mMutex;

        friend class Shader;
};
}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_SHADERPARAM_H