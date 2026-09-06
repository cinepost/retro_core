#include "texture.h"


namespace RetroLauncher {

Texture::Texture(): mTextureID(0), mWidth(0), mHeight(0), mWidthInv(0.0f), mHeightInv(0.0f), mLevel(0), mInternalFormat(GL_RGB32F),
    mpData(nullptr),
    mMinFilter(GL_LINEAR),
    mMagFilter(GL_LINEAR),
    mWrapModeS(GL_CLAMP_TO_BORDER),
    mWrapModeT(GL_CLAMP_TO_BORDER),

    mDirty(true),
    mParamsDirty(true)
{
    glGenTextures(1, &mTextureID);
    assert(mTextureID != 0);
}

Texture::Texture(GLsizei width, GLsizei height): Texture() {
    setSize(width, height);
}

void Texture::createBackingResource() {
    if(!mDirty) return;

    destroy();
    glBindTexture(GL_TEXTURE_2D, mTextureID);
    glTexImage2D(GL_TEXTURE_2D, mLevel, mInternalFormat, mWidth, mHeight, mBorder, mFormat, mType, mpData);
        
    mDirty = false;
}

void Texture::setParameters(bool force) {
    if(mParamsDirty || force) {
        
        // Set standard post-processing texture settings (Set Once!)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mMagFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWrapModeS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWrapModeT);
    }

    mParamsDirty = false;
}

void Texture::destroy() {
    if(mTextureID > 0) {
        glDeleteTextures(1, &mTextureID);
    }
    mDirty = true;
}

Texture::~Texture() {
    destroy();
}

}  // namespace RetroLauncher
