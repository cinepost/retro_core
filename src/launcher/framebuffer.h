#ifndef __RETRO_CORE_LAUNCHER_FRAMEBUFFER_H
#define __RETRO_CORE_LAUNCHER_FRAMEBUFFER_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <unordered_map>

#include "texture.h"

namespace RetroLauncher {

class Framebuffer {
    public:
        Framebuffer();
        Framebuffer(GLsizei width, GLsizei height);
        ~Framebuffer();

        void bind();
        void unbind();

        void setSize(GLsizei width, GLsizei height) {
            if(mTexture.getWidth() == width && mTexture.getHeight() == height) return;
            mTexture.setSize(width, height);
            mDirty = true;
        }

        GLsizei getWidth() const { return mTexture.getWidth(); }
        GLsizei getHeight() const { return mTexture.getHeight(); }

        void destroy();

        GLuint getFBO() const { return mFboID; }
        Texture& getTexture() { return mTexture; }
        const Texture& getTexture() const { return mTexture; }

    private:
        void createBackingResource();

        GLuint mFboID;

        Texture mTexture;
        bool mDirty;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_FRAMEBUFFER_H