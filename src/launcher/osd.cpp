#include "osd.h"
#include "os.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include <imgui.h>

#include "lodepng/lodepng.h"

#include <vector>

namespace RetroLauncher {

 OSD::OSD(): 
    mWidth(0), mHeight(0), mScreenCols(0), mScreenRows(0), mShader("OSD"), mVaoID(0), mVboID(0), mFboID(0), mFontTextureID(0), mOSDTextureID(0),
    mActiveVertexCount(0), mRebuildOsdBuffers(false), mInitialized(false), mTTF(false)
{
    mFontAtlasWidth = 0;
    mFontAtlasHeight = 0;
    mFontAtlasGridSizeX = 1;
    mFontAtlasGridSizeY = 1;

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

    //if(!generateAtlasAtRuntimePNG("fonts/vcr_font_12x16.png", mFontTextureID, 12, 16)) {
    //    mInitialized = false;
    //    return;
    //}

    if(!generateAtlasAtRuntimeTTF("fonts/vcr_font.ttf", mFontTextureID, 16, 16, 18)) {
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

    mScreenCols = mWidth / mCellWidth;
    mScreenRows = mHeight / mCellHeight;
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

void OSD::addText(int x, int y, const std::string& text, float r, float g, float b, float a) {
    if(text.empty()) return;
    OsdElement element;
    element.text = text;
    element.x = x; element.y = y;
    element.color[0] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, r)));
    element.color[1] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, g)));
    element.color[2] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, b)));
    element.color[3] = static_cast<uint8_t>(255.0f * std::min(1.0f, std::max(0.0f, a)));
    mOsdElements.push_back(element);
    mRebuildOsdBuffers = true;
}

void OSD::setText(int x, int y, const std::string& text, float r, float g, float b, float a) {
    mRebuildOsdBuffers = true;
    mOsdElements.clear();
    addText(x, y, text, r, g, b, a);
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

        std::vector<uint32_t> utf8_text = decodeUTF8(el.text);

        for (size_t i = 0; i < utf8_text.size(); ++i) {
            if(el.color[3] == 0) continue;

            OsdVertex v;
            v.x = startX + static_cast<float>(i * getGlyphWidth()); 
            v.y = startY; 

            if(mTTF) {
                auto it = mCodepointToGridID.find(utf8_text[i]);
                if (it != mCodepointToGridID.end()) {
                    v.charId = it->second;
                } else {
                    // Fallback to index for space character or 0
                    v.charId = mCodepointToGridID[' ']; 
                }
            } else {
               v.charId = static_cast<uint32_t>(el.text[i]) + 32;
            }
            
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

bool OSD::generateAtlasAtRuntimePNG(const std::string& fontPath, GLuint& outTexID, unsigned int cellWidth, unsigned int cellHeight) {
    assert(!fontPath.empty());
    assert(cellWidth > 0);
    assert(cellHeight > 0);

    outTexID = 0;

    std::vector<unsigned char> imageFileBytes;
    static const std::string sExecutableDir = getExecutableDir();

    fs::path finalPath(fontPath);
    if (finalPath.is_relative()) {
        finalPath = sExecutableDir / finalPath;
    }
    finalPath = fs::weakly_canonical(finalPath);

    if (lodepng::load_file(imageFileBytes, finalPath) != 0) {
        std::cerr << "Error OSD::generateAtlasAtRuntimePNG(...): Failed to open file " << finalPath << "\n";
        return false;
    }

    unsigned int imageWidth = 0;
    unsigned int imageHeight = 0;

    std::vector<unsigned char> atlasPixels; // Expect 8bpp indexed image
    unsigned error = lodepng::decode(atlasPixels, imageWidth, imageHeight, imageFileBytes, LCT_RGBA, 8);
    
    if (error) {
        std::cerr << "OSD::generateAtlasAtRuntimePNG(...): LodePNG error " << error << ": " << lodepng_error_text(error) << std::endl;
        return false;
    }

    if(atlasPixels.empty() || imageWidth == 0 || imageHeight == 0) {
        std::cerr << "OSD::generateAtlasAtRuntimePNG(...): Image " << fontPath << " has no data !" << std::endl;
        return false;
    }

    mCellWidth = cellWidth;
    mCellHeight = cellHeight;
    mFontAtlasGridSizeX = imageWidth / mCellWidth;
    mFontAtlasGridSizeY = imageHeight / mCellHeight;
    mFontAtlasWidth = imageWidth;
    mFontAtlasHeight = imageHeight;

    std::cout << "mCellWidth " << mCellWidth << std::endl;
    std::cout << "mCellHeight " << mCellHeight << std::endl;
    std::cout << "mFontAtlasGridSizeX " << mFontAtlasGridSizeX << std::endl;
    std::cout << "mFontAtlasGridSizeY " << mFontAtlasGridSizeY << std::endl;
    std::cout << "mFontAtlasWidth " << mFontAtlasWidth << std::endl;
    std::cout << "mFontAtlasHeight " << mFontAtlasHeight << std::endl;

    glGenTextures(1, &outTexID);
    glBindTexture(GL_TEXTURE_2D, outTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mFontAtlasWidth, mFontAtlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasPixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

bool OSD::generateAtlasAtRuntimeTTF(const std::string& fontPath, GLuint& outTexID, unsigned int cellWidth, unsigned int cellHeight, float fontPixelHeight) {
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

    mCellWidth = cellWidth;
    mCellHeight = cellHeight;

    std::vector<uint32_t> codepoints = getAllFontCodepoints(font);
    
    // A 16xXX grid of characters accommodates all 256 ASCII/Extended entries cleanly
    mFontAtlasGridSizeX = 16;
    mFontAtlasGridSizeY = codepoints.size() / mFontAtlasGridSizeX;
    mFontAtlasWidth = mFontAtlasGridSizeX * mCellWidth;   // 16 * 16 = 256 pixels wide
    mFontAtlasHeight = mFontAtlasGridSizeY * mCellHeight; // 16 * 16 = 256 pixels high

    std::cout << "mCellWidth " << mCellWidth << std::endl;
    std::cout << "mCellHeight " << mCellHeight << std::endl;
    std::cout << "mFontAtlasGridSizeX " << mFontAtlasGridSizeX << std::endl;
    std::cout << "mFontAtlasGridSizeY " << mFontAtlasGridSizeY << std::endl;
    std::cout << "mFontAtlasWidth " << mFontAtlasWidth << std::endl;
    std::cout << "mFontAtlasHeight " << mFontAtlasHeight << std::endl;


    // Create an empty memory buffer for a 128x128 monochrome atlas texture
    std::vector<unsigned char> atlasPixels(mFontAtlasWidth * mFontAtlasHeight, 0);

    // Calculate scaling factor to get our target pixel height
    float scale = stbtt_ScaleForPixelHeight(&font, fontPixelHeight);
    
    // Get baseline metrics so characters align vertically inside their cells
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    int baseline = static_cast<int>(ascent * scale);

    mCodepointToGridID.clear();

    // Populate all available character slots
    for (size_t i = 0; i < codepoints.size(); ++i) {
        if (i >= (mFontAtlasGridSizeX * mFontAtlasGridSizeY)) break; // Safety

        uint32_t cp = codepoints[i];

        int glyph = stbtt_FindGlyphIndex(&font, cp);
        if(glyph <= 0) continue;

        mCodepointToGridID[cp] = static_cast<uint32_t>(i);

        uint gridX = i % mFontAtlasGridSizeX;
        uint gridY = i / mFontAtlasGridSizeX;

        int pixelX = gridX * cellWidth;
        int pixelY = gridY * cellHeight;

        // Render single glyph
        int x0, y0, x1, y1;
        stbtt_GetGlyphBitmapBox(&font, glyph, scale, scale, &x0, &y0, &x1, &y1);

        // Center character horizontally within cell. Optional but keeps monospace layout uniform ya know
        int advance, lsb;
        stbtt_GetGlyphHMetrics(&font, glyph, &advance, &lsb);
        int glyphWidth = x1 - x0;
        int offsetX = (cellWidth - glyphWidth) / 2; 

        int outputOffset = (pixelY + baseline + y0) * mFontAtlasWidth + (pixelX + offsetX);

        stbtt_MakeGlyphBitmap(&font, &atlasPixels[outputOffset], glyphWidth, y1 - y0, mFontAtlasWidth, scale, scale, glyph);
    }

    glGenTextures(1, &outTexID);
    glBindTexture(GL_TEXTURE_2D, outTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mFontAtlasWidth, mFontAtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasPixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    mTTF = true;

    return true;
}

void OSD::drawGui() {
    //return;

    ImGui::Begin("OSD");

    ImGui::Text("Font Atlas: %ux%upx.",mFontAtlasWidth, mFontAtlasHeight);

    ImGui::BeginChild("Texture Atlas");
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();

    ImGui::Image(
        (ImTextureID)(uintptr_t)mFontTextureID, 
        viewport_panel_size, 
        ImVec2(0.0, 0.0), // Top-Left
        ImVec2(1.0, 1.0)  // Bottom-Right
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

std::vector<uint32_t> OSD::decodeUTF8(const std::string& str) {
    std::vector<uint32_t> codepoints;
    for (size_t i = 0; i < str.length();) {
        unsigned char cp = str[i];
        uint32_t res = 0;
        size_t len = 0;

        if (cp <= 0x7F) { res = cp; len = 1; }
        else if ((cp & 0xE0) == 0xC0) { res = cp & 0x1F; len = 2; }
        else if ((cp & 0xF0) == 0xE0) { res = cp & 0x0F; len = 3; }
        else if ((cp & 0xF8) == 0xF0) { res = cp & 0x07; len = 4; }
        else { i++; continue; } // Invalid UTF-8 jump

        if (i + len > str.length()) break;

        for (size_t j = 1; j < len; ++j) {
            res = (res << 6) | (static_cast<unsigned char>(str[i + j]) & 0x3F);
        }
        codepoints.push_back(res);
        i += len;
    }
    return codepoints;
}

std::vector<uint32_t> OSD::getAllFontCodepoints(const stbtt_fontinfo& font) {
    std::vector<uint32_t> codepoints;
    stbtt_uint8* data = font.data + font.fontstart;
    
    uint16_t numTables = ttUSHORT(data + 4);
    stbtt_uint8* tableDir = data + 12;
    stbtt_uint8* cmapTablePtr = nullptr;

    for (int i = 0; i < numTables; ++i) {
        if (tableDir[i * 16 + 0] == 'c' && tableDir[i * 16 + 1] == 'm' && 
            tableDir[i * 16 + 2] == 'a' && tableDir[i * 16 + 3] == 'p') {
            uint32_t offset = ttULONG(tableDir + i * 16 + 8);
            cmapTablePtr = font.data + offset;
            break;
        }
    }

    if (!cmapTablePtr) return codepoints;

    uint16_t numSubtables = ttUSHORT(cmapTablePtr + 2);
    stbtt_uint8* subtableRecord = cmapTablePtr + 4;

    for (int i = 0; i < numSubtables; ++i) {
        uint16_t platform_id = ttUSHORT(subtableRecord + i * 8);
        uint16_t encoding_id = ttUSHORT(subtableRecord + i * 8 + 2);
        uint32_t subtable_offset = ttULONG(subtableRecord + i * 8 + 4);

        // Windows Unicode BMP (3,1) or Unicode Full Repertoire (3,10) or Universal (0,x)
        if ((platform_id == 3 && (encoding_id == 1 || encoding_id == 10)) || platform_id == 0) {
            stbtt_uint8* subtable = cmapTablePtr + subtable_offset;
            uint16_t format = ttUSHORT(subtable);

            // --- Format 4: Standard 16-bit characters (<= 0xFFFF) ---
            if (format == 4) {
                uint16_t segCountX2 = ttUSHORT(subtable + 6);
                uint16_t segCount = segCountX2 / 2;
                stbtt_uint8* endCountPtr = subtable + 14;
                stbtt_uint8* startCountPtr = endCountPtr + segCountX2 + 2;

                for (int j = 0; j < segCount; ++j) {
                    uint32_t start = ttUSHORT(startCountPtr + j * 2);
                    uint32_t end = ttUSHORT(endCountPtr + j * 2);
                    if (start == 0xFFFF && end == 0xFFFF) continue;

                    for (uint32_t cp = start; cp <= end; ++cp) {
                        if (cp == 65533) continue; // Skip default replacement glyph
                        if (stbtt_FindGlyphIndex(&font, cp) > 0) {
                            codepoints.push_back(cp);
                        }
                    }
                }
            }
            // --- Format 12: Extended 32-bit characters (> 0xFFFF, Emojis, Symbols) ---
            else if (format == 12) {
                uint32_t num_groups = ttULONG(subtable + 12);
                stbtt_uint8* group_ptr = subtable + 16;

                for (uint32_t g = 0; g < num_groups; ++g) {
                    uint32_t start_cp = ttULONG(group_ptr + g * 12 + 0);
                    uint32_t end_cp   = ttULONG(group_ptr + g * 12 + 4);

                    for (uint32_t cp = start_cp; cp <= end_cp; ++cp) {
                        if (cp == 65533) continue;
                        if (stbtt_FindGlyphIndex(&font, cp) > 0) {
                            codepoints.push_back(cp);
                        }
                    }
                }
            }
        }
    }

    // Deduplicate just in case both platform tables contain matching overlapping segments
    std::sort(codepoints.begin(), codepoints.end());
    codepoints.erase(std::unique(codepoints.begin(), codepoints.end()), codepoints.end());

    return codepoints;
}

}  // namespace RetroLauncher
