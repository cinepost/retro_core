#include "crt.h"

#include <iostream>
#include <random>

namespace RetroLauncher {

CRT::CRT(): Display(), 
    mEncoderShader("Encoder"), 
    mDecoderShader("Decoder"), 
    mHistoryShader("History"), 
    mDisplayShader("Display"),
    mCoreTextureWidth(0), 
    mCoreTextureHeight(0), 
    mEncodedTextureWidth(0), 
    mEncodedTextureHeight(0), 
    mInitialized(false) 
{
    mpOSD = std::make_unique<OSD>();
}

void CRT::setStandard(Standard standard) {
    standard = static_cast<Standard>((uint32_t)standard % (uint32_t)Standard::COUNT);
    if(mStandard == standard) return;
    mStandard = standard;

    std::cout << "Video standard chaged to " << getStandardString() << std::endl;
}

void CRT::setMode(Mode mode) {
    mode = mode % Mode::COUNT;
    if(mMode == mode) return;
    mMode = mode;

    std::cout << "Video mode chaged to " << getModeString() << std::endl;

    // Zero to re-generate encoder-decoder texture
    mCoreTextureWidth = 0;
    mCoreTextureHeight = 0;
}

void CRT::prepareEncoderTexture(uint16_t core_tex_width, uint16_t core_tex_height) {
    if(mCoreTextureWidth == core_tex_width && mCoreTextureHeight == core_tex_height) return;

    mCoreTextureWidth = core_tex_width;
    mCoreTextureHeight = core_tex_height;

    uint16_t encoded_tex_width = 0;
    uint16_t encoded_tex_height = 0;

    switch(mMode) {
        case Mode::RF:
        case Mode::COMPOSITE:
        case Mode::S_VIDEO:
            encoded_tex_width = mCoreTextureWidth * 4;
            encoded_tex_height = mCoreTextureHeight;
            break;
        case Mode::COMPONENT:
            encoded_tex_width = mCoreTextureWidth * 2;
            encoded_tex_height = mCoreTextureHeight;
            break;
        case Mode::VGA:
        default:
            encoded_tex_width = mCoreTextureWidth * 2;
            encoded_tex_height = mCoreTextureHeight * 2;
            break;
    }

    mpOSD->resize(mpOSD->getWidth(), encoded_tex_height); // only vertical resize. keep OSD width fixed.

    auto encoded_tex_format = GL_RGB16F;
    switch(mMode) {
        case Mode::RF:
        case Mode::COMPOSITE:
            encoded_tex_format = GL_R16F;
            break;
        case Mode::S_VIDEO:
            encoded_tex_format = GL_RG16F;
            break;
        default:
            encoded_tex_format = GL_RGB16F;
            break;
    }

    if(mEncodedTextureWidth == encoded_tex_width && mEncodedTextureHeight == encoded_tex_height && mEncodedTextureFormat == encoded_tex_format) return;

    std::cout << "Generate encoder texture " << encoded_tex_width << "x" << encoded_tex_height << std::endl;

    static const float sBlackBorderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Encoded signal texture
    mEncodedTextureWidth  = encoded_tex_width;
    mEncodedTextureHeight = encoded_tex_height;
    mEncodedTextureFormat = encoded_tex_format;

    glDeleteTextures(1, &mEncodedTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, mEncoderFBO);
    glGenTextures(1, &mEncodedTexture);
    glBindTexture(GL_TEXTURE_2D, mEncodedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, encoded_tex_format, (GLsizei)mEncodedTextureWidth, (GLsizei)mEncodedTextureHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, sBlackBorderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mEncodedTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Decoder rgb texture
    mDecodedTextureWidth  = mMode == Mode::VGA ? 1024 : 2048;
    mDecodedTextureHeight = encoded_tex_height;

    glDeleteTextures(1, &mDecodedTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, mDecoderFBO);
    glGenTextures(1, &mDecodedTexture);
    glBindTexture(GL_TEXTURE_2D, mDecodedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, (GLsizei)mDecodedTextureWidth, (GLsizei)mDecodedTextureHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, sBlackBorderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mDecodedTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    // History (phosphor)
    mHistoryTextureWidth  = mDecodedTextureWidth;
    mHistoryTextureHeight = encoded_tex_height;

    for (int i = 0; i < 2; i++) {
        glDeleteTextures(1, &mHistoryTexture[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, mHistoryFBO[i]);
        glGenTextures(1, &mHistoryTexture[i]);
        glBindTexture(GL_TEXTURE_2D, mHistoryTexture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, (GLsizei)mHistoryTextureWidth, (GLsizei)mHistoryTextureHeight, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, sBlackBorderColor);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mHistoryTexture[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}



bool CRT::initImpl(uint16_t win_w, uint16_t win_h) {
    mInitialized = false;
    mWindowWidth = win_w;
    mWindowHeight = win_h;

    mpOSD->init(512, 288);

    std::vector<OsdElement> osd_elements;

    OsdElement elem1;

    elem1.text = "OSD test. 123..9 !@#$%^&*()";
    elem1.x = 0.0;
    elem1.y = 0.0; 
    elem1.duration = 1000;
    elem1.maxDuration = 1000;
    elem1.color[3] = 64;

    OsdElement elem2;

    elem2.text = "ZYX";
    elem2.x = 256;
    elem2.y = 144; 
    elem2.duration = 1000;
    elem2.maxDuration = 1000;

    osd_elements.push_back(elem1);
    osd_elements.push_back(elem2);

    mpOSD->setText(osd_elements);

    static const Shader::DefinesList sStandardTypeDefinesList = {
        {"NTSC",      "0u"},
        {"PAL",       "1u"}
    };

    static const Shader::DefinesList sConnectionTypeDefinesList = {
        {"RF",        "0u"},
        {"COMPOSITE", "1u"},
        {"S_VIDEO",   "2u"},
        {"COMPONENT", "3u"},
        {"VGA",       "4u"}
    };

    mEncoderShader.addDefines(sStandardTypeDefinesList);
    mEncoderShader.addDefines(sConnectionTypeDefinesList);
    if(!mEncoderShader.init("shaders/quad.vs", "shaders/encoder.fs")) {
        return false;
    }
    mEncoderShader.linkShaderParameter("uConnType", reinterpret_cast<uint32_t&>(mMode), 0, (uint32_t)Mode::COUNT - 1);
    mEncoderShader.linkShaderParameter("uStandard", reinterpret_cast<uint32_t&>(mStandard), 0, (uint32_t)Standard::COUNT - 1);

    mDecoderShader.addDefines(sStandardTypeDefinesList);
    mDecoderShader.addDefines(sConnectionTypeDefinesList);
    if(!mDecoderShader.init("shaders/quad.vs", "shaders/decoder.fs")) {
        return false;
    }
    mDecoderShader.linkShaderParameter("uConnType", reinterpret_cast<uint32_t&>(mMode), 0, (uint32_t)Mode::COUNT - 1);
    mDecoderShader.linkShaderParameter("uStandard", reinterpret_cast<uint32_t&>(mStandard), 0, (uint32_t)Standard::COUNT - 1);

    if(!mHistoryShader.init("shaders/quad.vs", "shaders/history.fs")) {
        return false;
    }
    
    mDisplayShader.addDefines(sStandardTypeDefinesList);
    mDisplayShader.addDefines(sConnectionTypeDefinesList);
    if(!mDisplayShader.init("shaders/quad.vs", "shaders/display.fs")) {
        return false;
    }
    mDisplayShader.linkShaderParameter("uConnType", reinterpret_cast<uint32_t&>(mMode), 0, (uint32_t)Mode::COUNT - 1);

    // Create intermediate frame buffers for Pass-2 input streams
    glGenFramebuffers(1, &mEncoderFBO);
    glGenFramebuffers(1, &mDecoderFBO);
    
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &mHistoryFBO[i]);
    }

    // Mapped Mesh structures
    static const float vertices[] = {
        -1.0f, -1.0f,  0.0f, 0.0f, 
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f, 1.0f,  1.0f, 1.0f, 
        -1.0f, 1.0f,  0.0f, 1.0f
    };
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    mInitialized = true;
    return mInitialized;
}

void CRT::destroy() {
    if(!mInitialized) return;

    if(mpOSD) mpOSD->destroy();

    glDeleteFramebuffers(1, &mEncoderFBO);
    glDeleteTextures(1, &mEncodedTexture);

    glDeleteFramebuffers(1, &mDecoderFBO);
    glDeleteTextures(1, &mDecodedTexture);

    for (int i = 0; i < 2; i++) {
        glDeleteFramebuffers(1, &mHistoryFBO[i]);
        glDeleteTextures(1, &mHistoryTexture[i]);
    }

    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
    
    mEncoderShader.destroy();
    mDecoderShader.destroy();
    mHistoryShader.destroy();
    mDisplayShader.destroy();

    mInitialized = false;
}

bool CRT::processImpl(GLuint core_texture, uint16_t core_tex_width, uint16_t core_tex_height) {
    if(!mInitialized) {
#ifdef DEBUG
    std::cerr << "[CRT] Display not initialized!" << std::endl;
#endif
        return false;
    }

    static const float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

#ifdef DEBUG
    reloadShaders();
#endif

    prepareEncoderTexture(core_tex_width, core_tex_height);

    GLboolean currentDepthTestState = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    if(mpOSD) {
        mpOSD->render();
    }

    // ====================================// Encoder: Render Game Texture to the FBO Buffer// ==========================================
    glBindFramebuffer(GL_FRAMEBUFFER, mEncoderFBO);
    glViewport(0, 0, mEncodedTextureWidth, mEncodedTextureHeight);
    mEncoderShader.use();

    static std::random_device rd; 
    static std::mt19937 gen(rd()); 
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    mEncoderShader.setFloat("uRandom", dis(gen));
    mEncoderShader.setInt("uSyncEnabled", 0);
    mEncoderShader.setFloat("uSyncLevel", 0.0);
    mEncoderShader.setInt("uFrameCount", (uint32_t)getFrontendFrameCount());
    mEncoderShader.setBool("uScandouble", mMode == Mode::VGA ? 1 : 0);
    mEncoderShader.setFloat("uInW", (float)mCoreTextureWidth);
    mEncoderShader.setFloat("uInH", (float)mCoreTextureHeight);
    mEncoderShader.setFloat("uEncW", (float)mEncodedTextureWidth);
    mEncoderShader.setFloat("uEncH", (float)mEncodedTextureHeight);
    mEncoderShader.setFloat("uOutW", (float)mEncodedTextureWidth);
    mEncoderShader.setFloat("uOutH", (float)mEncodedTextureHeight);
    mEncoderShader.pushTexture("uInputTex", core_texture, 0);
    
    glBindVertexArray(mVAO); 
    glDrawArrays(GL_QUADS, 0, 4);

    // ====================================// Decoder: Decode signal to the FBO Buffer// ==========================================
    glBindFramebuffer(GL_FRAMEBUFFER, mDecoderFBO);
    glViewport(0, 0, mDecodedTextureWidth, mDecodedTextureHeight);
    mDecoderShader.use();

    float misconvergenceX = mMisconvergenceX;
    if (mMode == Mode::VGA) {
        // Reduce RGB misconvergence in VGA mode 
        misconvergenceX *= 0.5;
    }

    mDecoderShader.setInt("uPALCombEnabled", 0);
    mDecoderShader.setInt("uPALCombEdgeProtect", 0);
    mDecoderShader.setInt("uSyncStripEnabled", 0);
    mDecoderShader.setFloat("uSyncLevel", 0.0);

    mDecoderShader.setFloat("uInW", (float)mEncodedTextureWidth);
    mDecoderShader.setFloat("uInH", (float)mEncodedTextureHeight);
    mDecoderShader.setFloat("uEncW", (float)mEncodedTextureWidth);
    mDecoderShader.setFloat("uEncH", (float)mEncodedTextureHeight);
    mDecoderShader.setFloat("uOutW", (float)mDecodedTextureWidth);
    mDecoderShader.setFloat("uOutH", (float)mDecodedTextureWidth);

    mDecoderShader.setInt("uFrameCount", getFrontendFrameCount());

    mDecoderShader.pushTexture("uInputTex", mEncodedTexture, 0);

    glBindVertexArray(mVAO); 
    glDrawArrays(GL_QUADS, 0, 4);

    // ====================================// History: Phosphor Decay Trailing Logic (Ring-Buffer) // ==========================================
    glBindFramebuffer(GL_FRAMEBUFFER, mHistoryFBO[mHistoryWriteIndex]);
    glViewport(0, 0, mHistoryTextureWidth, mHistoryTextureHeight);
    mHistoryShader.use();

    mHistoryShader.pushTexture("uDecodedTexture", mDecodedTexture, 0);
    mHistoryShader.pushTexture("uHistoryTexture", mHistoryTexture[mHistoryReadIndex], 1);

    glBindVertexArray(mVAO); 
    glDrawArrays(GL_QUADS, 0, 4);
    
    // ====================================// Present to screen via CRT Shader// ==========================================
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mWindowWidth, mWindowHeight);
    mDisplayShader.use();
    mDisplayShader.setVec2("uVideoResolution", (float)mHistoryTextureWidth, (float)mHistoryTextureHeight);
    mDisplayShader.setVec2("uOSDResolution", mpOSD->getWidth(), mpOSD->getHeight());
    mDisplayShader.setVec2("uOutResolution", (float)mWindowWidth, (float)mWindowHeight);
    mDisplayShader.setInt("uFrameCount", (uint32_t)getFrameCount());
    mDisplayShader.setUint("uFrontendFrameCount", (uint32_t)getFrontendFrameCount());
    mDisplayShader.pushTexture("uVideoTexture", mHistoryTexture[mHistoryReadIndex], 0);
    mDisplayShader.pushTexture("uOSDTexture", mpOSD->getOSDTextureID(), 1);

    glBindVertexArray(mVAO); 
    glDrawArrays(GL_QUADS, 0, 4);

    // restore state for legacy render
    glEnable(currentDepthTestState);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    mHistoryReadIndex = 1 - mHistoryReadIndex;
    mHistoryWriteIndex = 1 - mHistoryWriteIndex; 

    return true;
}

void CRT::reloadShaders() {
#ifdef DEBUG
    mEncoderShader.checkAndReload();
    mDecoderShader.checkAndReload();
    mHistoryShader.checkAndReload();
    mDisplayShader.checkAndReload();
#endif // DEBUG
}

void CRT::drawGuiImpl() {
    static const std::vector<Shader*> sShaders = {
        &mEncoderShader,
        &mDecoderShader,
        &mHistoryShader,
        &mDisplayShader
    };

    uint32_t selected_standard_idx = (uint32_t)getStandard();
    uint32_t mode_standard_idx = (uint32_t)getMode();

    if (ImGui::BeginCombo("Standard##dropdown", to_string(mStandard).c_str())) {
        for (uint32_t i = 0; i < (uint32_t)Standard::COUNT; ++i) {
            const bool is_selected = ((uint32_t)getStandard() == i);
            
            // Render each item as a Selectable
            if (ImGui::Selectable(to_string(static_cast<Standard>(i)).c_str(), is_selected)) {
                selected_standard_idx = i;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo(); // Must be called if BeginCombo returns true
    }

    if (ImGui::BeginCombo("Connection##dropdown", to_string(mMode).c_str())) {
        for (uint32_t i = 0; i < (uint32_t)Mode::COUNT; ++i) {
            const bool is_selected = ((uint32_t)getMode() == i);
            
            // Render each item as a Selectable
            if (ImGui::Selectable(to_string(static_cast<Mode>(i)).c_str(), is_selected)) {
                mode_standard_idx = i;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo(); // Must be called if BeginCombo returns true
    }

    if(ImGui::BeginTabBar(getDisplayName().c_str())) {

        for(Shader* pShader: sShaders) {
            ImGui::PushID(this);
            if (ImGui::BeginTabItem(pShader->getName().c_str())) {

                pShader->drawUI();        
                ImGui::EndTabItem();
            }
            ImGui::PopID();
        }

        ImGui::EndTabBar();
    }

    mpOSD->drawGui();
}

std::string CRT::getStandardString() const {
    return to_string(mStandard);
}

std::string CRT::getModeString() const {
    std::string str = to_string(mMode);
    if (mMode == CRT::Mode::VGA) return str;

    return str + " " + to_string(mStandard);
}

}  // namespace RetroLauncher