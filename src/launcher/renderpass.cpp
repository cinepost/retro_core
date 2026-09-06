#include "renderpass.h"

namespace RetroLauncher {

static const float sVertices[] = {
    -1.0f, -1.0f,  0.0f, 0.0f, 
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f, 1.0f,  1.0f, 1.0f, 
    -1.0f, 1.0f,  0.0f, 1.0f
};

RenderPass::RenderPass(const std::string& name): mShader(name), mInitialized(false) {
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sVertices), sVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

bool RenderPass::init(const std::string& vertPath, const std::string& fragPath) {
    mInitialized = mShader.init(vertPath, fragPath);
    return mInitialized;
}

void RenderPass::render() {
    mFramebuffer.bind();
    glViewport(0, 0, mFramebuffer.getWidth(), mFramebuffer.getHeight());
    glBindVertexArray(mVAO);

    mShader.use();
    setShaderParameters();

    glDrawArrays(GL_QUADS, 0, 4);
    mFramebuffer.unbind();
}

void RenderPass::renderToScreen(uint16_t win_w, uint16_t win_h) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win_w, win_h);
    glBindVertexArray(mVAO);
    
    mShader.use();
    setShaderParameters();

    glDrawArrays(GL_QUADS, 0, 4);
}

void RenderPass::setShaderParameters() const {
    // set inputs (textures)
    int i = 0;
    for (const auto& [id, resource] : mInputs) {
        std::visit([this, &i, id](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>) {
                assert(arg >= 0 && "OpenGL Texture ID cannot be negative!");
                mShader.pushTexture(id, arg, i++);
            } else if constexpr (std::is_same_v<T, Texture*>) {
                assert(arg != nullptr && "Texture pointer is null!");
                mShader.pushTexture(id, arg->getTextureID(), i++);
            } else if constexpr (std::is_same_v<T, RenderPass*>) {
                assert(arg != nullptr && "Dependent RenderPass pointer is null!");
                mShader.pushTexture(id, arg->getFramebuffer().getTexture().getTextureID(), i++);
            }
        }, resource);

    }
}

void RenderPass::destroy() {
    mInputs.clear();
    mShader.destroy();
    mFramebuffer.destroy();
}

RenderPass::~RenderPass() {
    destroy();

    if(mVAO > 0) {
        glDeleteVertexArrays(1, &mVAO);
        mVAO = 0;
    }

    if(mVBO > 0) {
        glDeleteBuffers(1, &mVBO);
        mVBO = 0;
    }
}


}  // namespace RetroLauncher
