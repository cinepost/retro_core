#include "crt.h"
#include "shaders.h"

namespace RetroLauncher {

CRT::CRT(): mInitialized(false) {

}

GLuint CRT::compileShaderProgram(const char* vert_src, const char* frag_src) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vert_src, nullptr); 
    glCompileShader(vs);
    
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag_src, nullptr); 
    glCompileShader(fs);
    
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

bool CRT::init(int win_w, int win_h) {
    mInitialized = false;
    mWinW = win_w;
    mWinH = win_h;

    mShaderPass1 = compileShaderProgram(vertex_shader_src, pass1_fragment_src);
    mShaderPass2 = compileShaderProgram(vertex_shader_src, pass2_fragment_src);

    // Create intermediate frame buffers for Pass-2 input streams
    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glGenTextures(1, &mFBOTexture);
    glBindTexture(GL_TEXTURE_2D, mFBOTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)mWinW, (GLsizei)mWinH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFBOTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    glDeleteFramebuffers(1, &mFBO);
    glDeleteTextures(1, &mFBOTexture);
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
    glDeleteProgram(mShaderPass1);
    glDeleteProgram(mShaderPass2);

    mInitialized = false;
}

bool CRT::process(GLuint core_texture, unsigned core_tex_width, unsigned core_tex_height) const {
    if(!mInitialized) return false;

    // ==========================================// PASS 1: Render Game Texture to the FBO Buffer// ==========================================
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glViewport(0, 0, core_tex_width, core_tex_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mShaderPass1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, core_texture);
    glUniform1i(glGetUniformLocation(mShaderPass1, "uCoreTexture"), 0);
    glBindVertexArray(mVAO); 
    glDrawArrays(GL_QUADS, 0, 4);

    // ==========================================// PASS 2: Render FBO to Monitor via CRT Shader// ==========================================
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mWinW, mWinH);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mShaderPass2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mFBOTexture);
    glUniform1i(glGetUniformLocation(mShaderPass2, "uCoreTexture"), 0);
    glUniform2f(glGetUniformLocation(mShaderPass2, "uCoreResolution"), (float)core_tex_width, (float)core_tex_height);
    glUniform2f(glGetUniformLocation(mShaderPass2, "uOutResolution"), (float)mWinW, (float)mWinH);
    glBindVertexArray(mVAO); 
    glDrawArrays(GL_QUADS, 0, 4);

    return true;
}

}  // namespace RetroLauncher