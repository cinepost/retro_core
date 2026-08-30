#ifndef __RETRO_CORE_LAUNCHER_SHADRES_H
#define __RETRO_CORE_LAUNCHER_SHADRES_H

// --- Shaders Source Definitions (GLSL 330 Core Profile Compliance) ---
const char* vertex_shader_src = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        TexCoord = aTexCoord;
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    }
)";

// Pass 1 Fragment: Basic texture layout color sampler Pass
const char* pass1_fragment_src = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D uCoreTexture;
    void main() {
        vec2 uv = TexCoord;
        uv.y = 1.0f - uv.y;
        FragColor = texture(uCoreTexture, uv);
    }
)";

// Pass 2 Fragment: Dynamic CRT Scanline and Screen Curvature Emulator
const char* pass2_fragment_src = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D uCoreTexture;
    uniform vec2 uCoreResolution;
    uniform vec2 uOutResolution;

    // Simulates an old curved cathode-ray glass tube surface
    vec2 curve(vec2 uv) {
        uv = (uv - 0.5) * 2.0;
        uv.x *= 1.0 + pow((abs(uv.y) / 5), 2.0);
        uv.y *= 1.0 + pow((abs(uv.x) / 4), 2.0);
        uv = (uv / 2.0) + 0.5;

        return uv;
    }

    void main() {
        vec2 uv = curve(TexCoord);
        
        // Edge blanking to cut off pixel bleeding past screen borders
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        vec2 core_tex_uv = uv * (uCoreResolution / uOutResolution);
        vec4 core_color = texture(uCoreTexture, core_tex_uv);
        
        // Dynamic scanline application calculation based on screen height
        float scanline = sin(uv.y * uOutResolution.y * 1.5) * 0.15;
        core_color.rgb -= scanline;

        // Subtle vintage phosphor color shadow bloom factor
        core_color.rgb *= 1.05; 
        
        FragColor = vec4(core_color.rgb, 1.0);
    }
)";

#endif  // __RETRO_CORE_LAUNCHER_SHADRES_H