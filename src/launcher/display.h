#ifndef __RETRO_CORE_LAUNCHER_DISPLAY_H
#define __RETRO_CORE_LAUNCHER_DISPLAY_H

#include "osd.h"

#include <imgui.h>

#include <memory>
#include <mutex>

namespace RetroLauncher {

class Display {
    public:
        Display(): mFrameCount(0), mFrontendFrameCount(0) {};

        virtual ~Display() {};

        virtual bool initImpl(uint16_t win_w, uint16_t win_h) = 0;
        virtual bool processImpl(GLuint core_texture, uint16_t core_tex_width, uint16_t core_tex_height) = 0;
        virtual void destroy() = 0;
        virtual const std::string& getDisplayName() const = 0;

        virtual void drawGuiImpl() = 0;

        bool init(uint16_t win_w, uint16_t win_h) {
            bool result = initImpl(win_w, win_h);
            mFrameCount = 0;
            return result;
        }

        bool process(GLuint core_texture, uint16_t core_tex_width, uint16_t core_tex_height){
            bool result = processImpl(core_texture, core_tex_width, core_tex_height);
            mFrameCount++;
            return result;
        }

        void drawUI() {
            ImGui::Separator();
            ImGui::Text("Shaders: %s", getDisplayName().c_str());
            if(ImGui::BeginTabBar(getDisplayName().c_str())) {
                drawGuiImpl();
                ImGui::EndTabBar();
            }
        }

        virtual OSD* getOSD() { return nullptr; }

        size_t getFrameCount() const { return mFrameCount; }

        void setFrontendFrameCount(size_t frame_number) { 
            std::lock_guard<std::mutex> lock(mFrontendMutex);
            mFrontendFrameCount = frame_number; 
        }
        
        size_t getFrontendFrameCount() const { 
            std::lock_guard<std::mutex> lock(mFrontendMutex);
            return mFrontendFrameCount; 
        }
        const size_t* getFrontendFrameCountPtr() const { 
            std::lock_guard<std::mutex> lock(mFrontendMutex);
            return &mFrontendFrameCount; 
        }

    private:
        size_t mFrameCount;
        size_t mFrontendFrameCount;

        mutable std::mutex mFrontendMutex;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_DISPLAY_H