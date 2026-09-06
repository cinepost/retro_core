#ifndef __RETRO_CORE_LAUNCHER_TEXTURE_H
#define __RETRO_CORE_LAUNCHER_TEXTURE_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <unordered_map>



namespace RetroLauncher {

class Texture {
    public:
        Texture();
        Texture(GLsizei width, GLsizei height);
        ~Texture();

        void destroy();
        GLuint getTextureID() const { return mTextureID; }
        void bind() {
            assert(mTextureID != 0);
            createBackingResource();
            setParameters();
            glBindTexture(GL_TEXTURE_2D, mTextureID);
        }

        void setSize(GLsizei width, GLsizei height) {
            if(mWidth == width || mHeight == height) return;
            mWidth = width;
            mHeight = height;
            mWidthInv = 1.0 / float(std::max(0, mWidth));
            mHeightInv = 1.0 / float(std::max(0, mHeight));
            makeDirty();
        }

        void setBorder(GLint border) {
            if(mBorder == border) return;
            mBorder = border;
            makeDirty();
        }

        void setFormat(GLenum format) {
            if(mFormat == format) return;
            mFormat = format;
            makeDirty();
        }

        void setType(GLenum type) {
            if(mType == type) return;
            mType = type;
            makeDirty();
        }

        void setLevel(GLint level) {
            if(mLevel == level) return;
            mLevel = level;
            makeDirty();
        }

        void setInternalFormat(GLint format) {
            if(mInternalFormat == format) return;
            mInternalFormat = format;
            makeDirty();
        }

        void setData(const void* pData) {
            if(mpData == pData) return;
            mpData = pData;
            makeDirty();
        }

        // Texture params
        void setFilter(GLint min_param, GLint mag_param) {
            setMinFilter(min_param);
            setMagFilter(mag_param);
        }

        void setMinFilter(GLint param) {
            if(mMinFilter == param) return;
            mMinFilter = param;
            mParamsDirty = true;
        }

        void setMagFilter(GLint param) {
            if(mMagFilter == param) return;
            mMagFilter = param;
            mParamsDirty = true;
        }

        void setWrap(GLint s_mode, GLint t_mode) {
            setWrapS(s_mode);
            setWrapT(t_mode);
        }

        void setWrapS(GLint mode) {
            if(mWrapModeS == mode) return;
            mWrapModeS = mode;
            mParamsDirty = true;
        }

        void setWrapT(GLint mode) {
            if(mWrapModeT == mode) return;
            mWrapModeT = mode;
            mParamsDirty = true;
        }

        GLsizei getWidth() const { return mWidth; }
        GLsizei getHeight() const { return mHeight; }

    private:
        void init();
        void createBackingResource();
        void setParameters(bool force = false);

        void makeDirty() {
            mDirty = true;
        }

    private:
        GLuint mTextureID;

        GLsizei mWidth;
        GLsizei mHeight;
        float   mWidthInv;
        float   mHeightInv;

        GLint   mLevel;
        GLint   mInternalFormat;

        GLint   mBorder;
        GLenum  mFormat;
        GLenum  mType;

        const void*  mpData;

        // params
        GLint   mMinFilter;
        GLint   mMagFilter;
        GLint   mWrapModeS;
        GLint   mWrapModeT;


        // flags
        bool  mDirty;
        bool  mParamsDirty;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_TEXTURE_H