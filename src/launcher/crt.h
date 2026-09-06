#ifndef __RETRO_CORE_LAUNCHER_CRT_H
#define __RETRO_CORE_LAUNCHER_CRT_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <string>

#include "shader.h"
#include "display.h"


namespace RetroLauncher {

class CRT: public Display {
    public:
        enum class Standard: uint32_t {
            NTSC  = 0,
            PAL   = 1,
            COUNT = 2
        };

        enum class Mode: uint32_t {
            RF         = 0,
            COMPOSITE  = 1,
            COMPONENT  = 2,
            VGA        = 3,
            COUNT      = 4
        };

        CRT();

        virtual bool initImpl(uint16_t win_w, uint16_t win_h) override final;
        virtual bool processImpl(GLuint core_texture, uint16_t core_tex_width, uint16_t core_tex_height) override final;
        virtual void destroy() override final;
        virtual const std::string& getDisplayName() const override final {
            static const std::string sName = "CRT";
            return sName;
        }

        virtual void drawGuiImpl() override final;

        void setStandard(Standard standard);
        Standard getStandard() const { return mStandard; }
        std::string getStandardString() const;

        void setMode(Mode mode);
        Mode getMode() const { return mMode; }
        std::string getModeString() const;

    private:
        void prepareEncoderTexture(uint16_t core_tex_width, uint16_t core_tex_height);
        void reloadShaders();

    private:
        Shader mEncoderShader; // Core Output Prep / Signal Encoder Pass 
        Shader mDecoderShader; // Signal Decoder Pass 
        Shader mHistoryShader; // Holds the accumulated decay RGB signal
        Shader mDisplayShader; // CRT Physical Screen Geometry & Mask Shader

        //Framebuffer mEncoderFramebuffer;

        // Holds the NTSC/APL composite signal 
        // Format: GL_R32F for RF/Composite or GL_RGB32F for Component/VGA support.
        GLuint mEncoderFBO = 0;
        GLuint mEncodedTexture = 0;
        GLint  mEncodedTextureFormat = GL_RGB16F;

        // Holds the immediate decoded RGB signal before decay
        GLuint mDecoderFBO = 0;
        GLuint mDecodedTexture = 0;

        // Holds the accumulated decay RGB signal
        // Ping-Pong Framebuffer Ring
        GLuint mHistoryFBO[2];
        GLuint mHistoryTexture[2];

        GLuint mVAO = 0; 
        GLuint mVBO = 0;

        uint16_t mCoreTextureWidth;
        uint16_t mCoreTextureHeight;

        uint16_t mEncodedTextureWidth;
        uint16_t mEncodedTextureHeight;

        uint16_t mDecodedTextureWidth;
        uint16_t mDecodedTextureHeight;

        uint16_t mHistoryTextureWidth;
        uint16_t mHistoryTextureHeight;

        uint16_t mWindowWidth;
        uint16_t mWindowHeight;

        Mode mMode = Mode::COMPONENT;
        Standard mStandard = Standard::NTSC;

        int mHistoryReadIndex = 0;
        int mHistoryWriteIndex = 1;
        float mMisconvergenceX = 0.1f;

        bool mInitialized;
};

constexpr CRT::Standard operator%(CRT::Standard lhs, uint8_t rhs) {
    return static_cast<CRT::Standard>(static_cast<uint8_t>(lhs) % rhs);
}

constexpr CRT::Standard operator%(CRT::Standard lhs, CRT::Standard rhs) {
    return static_cast<CRT::Standard>(static_cast<uint8_t>(lhs) % static_cast<uint8_t>(rhs));
}

constexpr CRT::Mode operator%(CRT::Mode lhs, uint8_t rhs) {
    return static_cast<CRT::Mode>(static_cast<uint8_t>(lhs) % rhs);
}

constexpr CRT::Mode operator%(CRT::Mode lhs, CRT::Mode rhs) {
    return static_cast<CRT::Mode>(static_cast<uint8_t>(lhs) % static_cast<uint8_t>(rhs));
}


}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_CRT_H