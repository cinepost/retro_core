#ifndef __RETRO_CORE_LAUNCHER_RENDERPASS_H
#define __RETRO_CORE_LAUNCHER_RENDERPASS_H

#include "shader.h"
#include "framebuffer.h"

#include <variant>


namespace RetroLauncher {

class RenderPass;

class RenderPass {
    public:
        using Input = std::variant<int, Texture*, RenderPass*>;

        RenderPass(const std::string& name);
        ~RenderPass();

        void render();
        void renderToScreen(uint16_t win_w, uint16_t win_h);

        bool init(const std::string& vertPath, const std::string& fragPath);

        void setOutputSize(GLsizei width, GLsizei height) {
            mFramebuffer.setSize(width, height);
        }

        void setResource(const std::string& name, Input resource) {
            std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_pointer_v<T>) {
                    assert(arg != nullptr && "Cannot assign a null pointer to a RenderPass resource!");
                }
            }, resource);

            mInputs[name] = resource;
        }

        template<typename T>
        T* getResource(const std::string& name) {
            auto it = mInputs.find(name);
            if (it != mInputs.end()) {
                
                std::visit([](auto&& arg) {
                    using TT = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_pointer_v<TT>) {
                        assert(arg != nullptr && "Renderpass input is a null pointer!");
                    }
                }, it->second);

                return std::get_if<T>(&it->second);
            }
            return nullptr;
        }

        const Shader& getShader() const { return mShader; }
        const Framebuffer& getOutput() const { return mFramebuffer; }

        GLsizei getOutputWidth() const { return mFramebuffer.getTexture().getWidth(); }
        GLsizei getOutputHeight() const { return mFramebuffer.getTexture().getHeight(); }

        void destroy();

        const Framebuffer& getFramebuffer() const { return mFramebuffer; }

    private:
        void setShaderParameters() const;

        Shader mShader;
        Framebuffer mFramebuffer;

        GLuint mVAO; 
        GLuint mVBO;

        bool mInitialized;
 
        std::unordered_map<std::string, Input> mInputs;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_RENDERPASS_H