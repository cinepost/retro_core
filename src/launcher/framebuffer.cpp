#include "framebuffer.h"


namespace RetroLauncher {

Framebuffer::Framebuffer(): mFboID(0), mDirty(true) {
    glGenFramebuffers(1, &mFboID);
}

Framebuffer::Framebuffer(GLsizei width, GLsizei height): Framebuffer() {
    setSize(width, height);
}

void Framebuffer::bind() {
    assert(mFboID != 0);

    createBackingResource();
    glBindFramebuffer(GL_FRAMEBUFFER, mFboID);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::createBackingResource() {
    if(!mDirty) return;

    glBindFramebuffer(GL_FRAMEBUFFER, mFboID);
    mTexture.bind();

    assert(mTexture.getTextureID() != 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture.getTextureID(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FBO Error] Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mDirty = false;
}

void Framebuffer::destroy() {
    mTexture.destroy();
    if(mFboID == 0) {
        glDeleteFramebuffers(1, &mFboID);
    }
}

Framebuffer::~Framebuffer() {
    destroy();
}


}  // namespace RetroLauncher
