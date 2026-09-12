#include "osd.h"
#include "os.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include <imgui.h>

#include <vector>

namespace RetroLauncher {

 OSD::OSD(): 
    mWidth(0), mHeight(0), mShader("OSD"), mVaoID(0), mVboID(0), mFboID(0), mFontTextureID(0), mOSDTextureID(0),
    mActiveVertexCount(0), mRebuildOsdBuffers(false), mInitialized(false) 
{
    mFontAtlasWidth = 0;
    mFontAtlasHeight = 0;
    mFontAtlasGridSizeX = 1;
    mFontAtlasGridSizeY = 1;

    mScreenCols = 32; // 40
    mScreenRows = 16; // 20

    mFrameCount = 0;
 };

void OSD::init(uint32_t width, uint32_t height) {
    if(mInitialized) return;

    if(width < 1 || height < 1) return;

    if(!mShader.init("shaders/osd.vs", "shaders/osd.fs", "shaders/osd.gs")) {
        std::cerr << "[OSD] Error initializing shader." << std::endl;
    }

    glGenFramebuffers(1, &mFboID);

    glGenVertexArrays(1, &mVaoID);
    glGenBuffers(1, &mVboID);

    glBindVertexArray(mVaoID);
    glBindBuffer(GL_ARRAY_BUFFER, mVboID);

    // Dynamic draw since text strings change frame-by-frame
    glBufferData(GL_ARRAY_BUFFER, sizeof(OsdVertex) * 2048, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(OsdVertex), (void*)offsetof(OsdVertex, x));

    // Character ID attribute (Integer mapping)
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(OsdVertex), (void*)offsetof(OsdVertex, charId));

    // Location 2: Color (4 Normalized Bytes -> converts 0-255 to 0.0-1.0 automatically in GLSL)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(OsdVertex), (void*)offsetof(OsdVertex, r));

    glBindVertexArray(0);

    if(!generateAtlasAtRuntime("fonts/vcr_font.ttf", mFontTextureID, 18, 18, 18)) {
        mInitialized = false;
        return;
    }

    resize(width, height);
    mInitialized = true;
}

void OSD::resize(uint32_t width, uint32_t height) {
    if(mWidth == width && mHeight == height) return;

    glBindFramebuffer(GL_FRAMEBUFFER, mFboID);

    glGenTextures(1, &mOSDTextureID);
    glBindTexture(GL_TEXTURE_2D, mOSDTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mOSDTextureID, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OSD FBO Error] Framebuffer creation failed!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mWidth = width;
    mHeight = height;
}

uint32_t OSD::addElement() {
    uint32_t id = mOsdElements.size();
    mOsdElements.push_back({});
    mRebuildOsdBuffers = true;
    return id;
} 

uint32_t OSD::addElement(const OsdElement& element) {
    uint32_t id = mOsdElements.size();
    mOsdElements.push_back(element);
    mRebuildOsdBuffers = true;
    return id;
}

void OSD::addText(int x, int y, const std::string& str, float r, float g, float b, float a) {
    if(str.empty()) return;
    OsdElement element;
    element.text = str;
    element.x = x; element.y = y;
    element.color[0] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, r)));
    element.color[1] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, g)));
    element.color[2] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, b)));
    element.color[3] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, a)));
    mOsdElements.push_back(element);
    mRebuildOsdBuffers = true;
}

void OSD::setText(int x, int y, const std::string& str, float r, float g, float b, float a) {
    mRebuildOsdBuffers = true;
    mOsdElements.clear();
    addText(x, y, str, r, g, b, a);
}

void OSD::setText(const OsdElement& element) {
    mRebuildOsdBuffers = true;
    mOsdElements.clear();
    mOsdElements.push_back(element);
}
void OSD::setText(const std::vector<OsdElement>& elements) {
    mRebuildOsdBuffers = true;
    mOsdElements = elements;
}

void OSD::updateBuffers() {
    if(!mRebuildOsdBuffers) return;
    std::vector<OsdVertex> vertices;

    for (const auto& el : mOsdElements) {
        float startX = std::floor(el.x); 
        float startY = std::floor(el.y);

        for (size_t i = 0; i < el.text.size(); ++i) {
            if(el.color[3] == 0) continue;

            OsdVertex v;
            v.x = startX + static_cast<float>(i * getGlyphWidth()); 
            v.y = startY; 
            v.charId = static_cast<uint32_t>(el.text[i]);
            v.r = el.color[0]; v.g = el.color[1]; v.b = el.color[2]; v.a = el.color[3];

            vertices.push_back(v);
        }
    }

    mActiveVertexCount = vertices.size(); 
    if (mActiveVertexCount != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, mVboID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(OsdVertex) * vertices.size(), vertices.data());
    } else {

    }

    mRebuildOsdBuffers = false;
}

void OSD::clear() {
    if(mOsdElements.empty()) return;
    mOsdElements.clear();
    mRebuildOsdBuffers = true;
}

void OSD::render() {
    assert(mInitialized);
    updateBuffers();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBindFramebuffer(GL_FRAMEBUFFER, mFboID);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (mActiveVertexCount == 0) {
        return;
    }

    glViewport(0, 0, mWidth, mHeight);
    mShader.use();
    glBindVertexArray(mVaoID);
    glBindBuffer(GL_ARRAY_BUFFER, mVboID);

    // Set uniform sizes and colors

    float glyphWidth = static_cast<float>(getGlyphWidth());
    float glyphHeight = static_cast<float>(getGlyphHeight());

    mShader.setVec2("uGlyphSize", glyphWidth, glyphHeight); 
    mShader.setVec2("uAtlasGridSize", (float)mFontAtlasGridSizeX, (float)mFontAtlasGridSizeY);
    mShader.setVec2("uScreenResolution", (float)mWidth, (float)mHeight); 
    mShader.setVec3("uTintColor", 1.0f, 1.0f, 1.0f);
    mShader.setBool("uDoShadow", true);
    mShader.pushTexture("fontAtlas", mFontTextureID, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mFontTextureID);

    // Draw your text elements expanded on-the-fly by the GPU
    glDrawArrays(GL_POINTS, 0, mActiveVertexCount);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mFrameCount++;
}

void OSD::drawDebug(int win_w, int win_h) {
    glColor3f(1.0f, 1.0f, 1.0f);

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mFontTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glViewport(0, 0, win_w, win_h);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

bool OSD::generateAtlasAtRuntime(const std::string& fontPath, GLuint& outTexID, int cellWidth, int cellHeight, float fontPixelHeight) {
    outTexID = 0;

    static const std::string sExecutableDir = getExecutableDir();

    fs::path finalPath(fontPath);
    if (finalPath.is_relative()) {
        finalPath = sExecutableDir / finalPath;
    }
    finalPath = fs::weakly_canonical(finalPath);

    std::vector<unsigned char> fontBuffer = readBinaryFile(finalPath);
    if(fontBuffer.empty()) {
        std::cerr << "Error. " << finalPath << " file is empty!" << std::endl;
        return false;
    }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(), 0)) {
        std::cerr << "[Font Error] Failed to initialize stb_truetype font info.\n";
        return false;
    }

    // A 16x16 grid of characters accommodates all 256 ASCII/Extended entries cleanly
    mFontAtlasGridSizeX = 16;
    mFontAtlasGridSizeY = 16;
    mFontAtlasWidth = mFontAtlasGridSizeX * cellWidth;   // 16 * 16 = 256 pixels wide
    mFontAtlasHeight = mFontAtlasGridSizeY * cellHeight; // 16 * 16 = 256 pixels high


    // Create an empty memory buffer for a 128x128 monochrome atlas texture
    std::vector<unsigned char> atlasPixels(mFontAtlasWidth * mFontAtlasHeight, 0);

    // Calculate scaling factor to get our target pixel height
    float scale = stbtt_ScaleForPixelHeight(&font, fontPixelHeight);
    
    // Get baseline metrics so characters align vertically inside their cells
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    int baseline = static_cast<int>(ascent * scale);

    // Populate all 256 character slots
    for (int charId = 0; charId < 256; ++charId) {
        uint gridX = charId % mFontAtlasGridSizeX;
        uint gridY = charId / mFontAtlasGridSizeX;

        // Render the single glyph from vectors to a temporary tight monochromatic bitmap
        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char* glyphBitmap = stbtt_GetCodepointBitmap(&font, 0, scale, charId, &w, &h, &xoff, &yoff);

        if (glyphBitmap) {
            // Determine initial centering offsets inside our rigid monospace tile box
            // 'baseline + yoff' positions the letter properly on its vertical font axis
            int targetXOffset = (cellWidth - w) / 2; // Center horizontally inside the cell
            int targetYOffset = baseline + yoff;    // Align vertically via font baseline
            
            // Clamp target offsets to prevent memory corruption if a glyph spills over
            if (targetXOffset < 0) targetXOffset = 0;
            if (targetYOffset < 0) targetYOffset = 0;

            // Blit the tight glyph pixels into our large monospace grid atlas buffer
            for (int y = 0; y < h; ++y) {
                if (targetYOffset + y >= cellHeight) break; // Exceeds cell boundary safety clamp

                for (int x = 0; x < w; ++x) {
                    if (targetXOffset + x >= cellWidth) break;

                    int destX = (gridX * cellWidth) + targetXOffset + x;
                    int destY = (gridY * cellHeight) + targetYOffset + y;

                    atlasPixels[destY * mFontAtlasWidth + destX] = glyphBitmap[y * w + x];
                }
            }

            // Free the memory buffer allocated by stb_truetype for this character
            stbtt_FreeBitmap(glyphBitmap, nullptr);
        }
    }

    glGenTextures(1, &outTexID);
    glBindTexture(GL_TEXTURE_2D, outTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mFontAtlasWidth, mFontAtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasPixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void OSD::drawGui() {
    return;

    ImGui::Begin("OSD");

    ImGui::Text("Font Atlas: %ux%upx.",mFontAtlasWidth, mFontAtlasHeight);

    ImGui::BeginChild("Texture Atlas");
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();

    ImGui::Image(
        (ImTextureID)(uintptr_t)mFontTextureID, 
        viewport_panel_size, 
        ImVec2(0.0, 1.0), // Top-Left
        ImVec2(1.0, 0.0)  // Bottom-Right
    );

    ImGui::EndChild();
    ImGui::End();
}

void OSD::destroy() {
    mShader.destroy();
    glDeleteFramebuffers(1, &mFboID); mFboID = 0;
    glDeleteTextures(1, &mFontTextureID); mFontTextureID = 0;
    glDeleteTextures(1, &mOSDTextureID); mOSDTextureID = 0;

    glDeleteVertexArrays(1, &mVaoID); mVaoID = 0;
    glDeleteBuffers(1, &mVboID); mVboID = 0;

    mInitialized = false;
}

OSD::~OSD() {
    destroy();
}

}  // namespace RetroLauncher
