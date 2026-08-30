#ifndef __RETRO_CORE_LAUNCHER_CRT_H
#define __RETRO_CORE_LAUNCHER_CRT_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

namespace RetroLauncher {

class CRT {
    public:
        CRT();

        bool init(int win_w, int win_h);
        bool process(GLuint core_texture, unsigned core_tex_width, unsigned core_tex_height) const;

        void destroy();

        static GLuint compileShaderProgram(const char* vert_src, const char* frag_src); // shader compiler utility

    private:
        GLuint mShaderPass1 = 0; // Pass 1: Core Output Prep / Scaler
        GLuint mShaderPass2 = 0; // Pass 2: The CRT Physical Screen Geometry & Mask Shader

        GLuint mFBO = 0;
        GLuint mFBOTexture = 0;
        GLuint mVAO = 0; 
        GLuint mVBO = 0;

        int mWinW;
        int mWinH;

        bool mInitialized;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_CRT_H