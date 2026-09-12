#ifndef __RETRO_CORE_LAUNCHER_OSD_H
#define __RETRO_CORE_LAUNCHER_OSD_H

#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

#include <string>

#include "shader.h"


namespace RetroLauncher {

struct OsdVertex {
    float x, y;       // Normalized screen or text-grid position (-1 to 1)
    uint32_t charId;  // ASCII character code (e.g., 65 for 'A')
    uint8_t r, g, b, a;

    OsdVertex() {}
};

struct OsdElement {
    static const uint32_t kInvalidID = std::numeric_limits<uint32_t>::max();
    enum class Alignment {
        CENTER,
        TOP,
        TOP_LEFT,
        TOP_RIGHT,
        LEFT,
        RIGHT,
        BOTTOM,
        BOTTOM_LEFT,
        BOTTOM_RIGHT
    };

    std::string text;
    float x, y;          // Pixel space coordinates
    float duration;      // Remaining time to show in seconds
    float maxDuration;   // Initial time for fade calculations
    uint8_t color[4];
    Alignment alignment;

    OsdElement(): alignment(Alignment::CENTER) {
        std::memset(&color[0], 255, 4);
    }
};

class OSD {
    public:
        OSD();
        ~OSD();

        virtual void destroy();
        virtual void init(uint32_t width, uint32_t height);
        void resize(uint32_t width, uint32_t height); 
        virtual void render();
        virtual void drawGui();

        virtual void clear();
        GLuint getOSDTextureID() const { return mOSDTextureID; }
        GLuint getFontAtlasTextureID() const { return mFontTextureID; }

        virtual uint32_t getWidth() const { return mWidth; }
        virtual uint32_t getHeight() const { return mHeight; }

        virtual uint32_t getGlyphWidth() const { return 18; }
        virtual uint32_t getGlyphHeight() const { return 18; }

        OsdElement& getElement(uint32_t id) {
            assert(id < mOsdElements.size());
            mRebuildOsdBuffers = true;
            return mOsdElements[id];
        }

        const OsdElement& getElement(uint32_t id) const {
            assert(id < mOsdElements.size());
            return mOsdElements[id];
        }

        uint32_t addElement();
        uint32_t addElement(const OsdElement& element);
        void addText(int x, int y, const std::string& string, float r = 0.0f, float g = 1.0f, float b = 0.0f, float a = 1.0f);
        void setText(int x, int y, const std::string& string, float r = 0.0f, float g = 1.0f, float b = 0.0f, float a = 1.0f);
        void setText(const OsdElement& element);
        void setText(const std::vector<OsdElement>& elements);

        size_t getActiveVertexCount() const { return mActiveVertexCount; }

        bool generateAtlasAtRuntime(const std::string& fontPath, GLuint& outTexID, int cellWidth = 16, int cellHeight = 16, float fontPixelHeight = 14.0f);

    private:
        void updateBuffers();
        void drawDebug(int win_w, int win_h);

    private:
        uint32_t mWidth;
        uint32_t mHeight;

        std::vector<OsdElement> mOsdElements;
        uint32_t mScreenCols;
        uint32_t mScreenRows;

        Shader mShader;
        GLuint mVaoID, mVboID, mFboID;
        GLuint mFontTextureID;
        GLuint mOSDTextureID;

        uint32_t mFontAtlasWidth;
        uint32_t mFontAtlasHeight;
        uint32_t mFontAtlasGridSizeX;
        uint32_t mFontAtlasGridSizeY;

        size_t mActiveVertexCount;

        bool mRebuildOsdBuffers;
        bool mInitialized;

        size_t mFrameCount;
};

}  // namespace RetroLauncher

#endif  // __RETRO_CORE_LAUNCHER_OSD_H