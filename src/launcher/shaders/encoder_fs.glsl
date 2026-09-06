#version 330 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uInputTex;
uniform uint   uConnType;      // 0=RF, 1=Composite, 2=Component, 3=RGB
uniform uint   uStandard;      // 0=NTSC, 1=PAL
uniform int    uSyncEnabled;   // 1=inject sync pulses
uniform float  uSyncLevel;     // -0.3 to 0.0 (sync tip)
uniform int    uScandouble;    // Applies only to VGA/HDMI modes
uniform float  uInW, uInH;
uniform float  uOutW, uOutH;
uniform float  uEncW, uEncH;

#include "common.glsl"
#include "sampling.glsl"

vec3 rgb2ycbccr(vec3 c) {
    float Y = 0.299*c.r + 0.587*c.g + 0.114*c.b;
    float Cb = 0.564*(c.b - Y);
    float Cr = 0.713*(c.r - Y);
    return vec3(Y, Cb, Cr);
}

void main() {
    float x = vTexCoord.x * uEncW;
    float y = vTexCoord.y * uEncH;
    int line = int(floor(y));
    float is_odd = mod(line, 2.0) > 0.5 ? 1.0 : 0.0;

    // Dynamic bandwidth scaling (luma)
    float lumaBW = uStandard == 0u ? 4.2 : 5.0;
    float sigmaY = clamp(lumaBW * (uEncW / 1000000.0) / 2.5, 0.5, uEncW/2.0);

    // 7-tap horizontal Gaussian filter
    float Y=0.0, Cb=0.0, Cr=0.0, wSum=0.0;
    for(int i=-3; i<=3; i++) {
        float dx = float(i) / uOutW; //uInW;
        vec2 uv = clamp(vTexCoord + vec2(dx, 0.0), vec2(0.0), vec2(1.0));

        //vec3 yc = rgb2ycbccr(texture(uInputTex, uv).rgb);
        vec3 yc = rgb2ycbccr(sampleScanlineLinear(uInputTex, uv.x, line, vec2(uInW, uInH)));
        
        float w = exp(-0.5 * (dx * uInW / sigmaY) * (dx * uInW / sigmaY));
        Y += yc.r * w; Cb += yc.g * w; Cr += yc.b * w; wSum += w;
    }
    Y /= wSum; Cb /= wSum; Cr /= wSum;

    // FIXED: Physically accurate phase (cycles per line × normalized x)
    float cycles_per_line = (uStandard == 0u ? 227.5 : 283.752);
    float phase = 2.0 * 3.14159265358979 * cycles_per_line * vTexCoord.x;
    
    // PAL: 180° phase alternation on odd lines
    if(uStandard == 1u) phase += mod(line, 2.0) * 3.14159265358979;

    // Quadrature modulation
    float chroma = Cb * sin(phase) + Cr * cos(phase);
    float signal = clamp(Y + chroma, 0.0, 1.0);

    // Sync injection (fixed step syntax)
    float h_sync_ratio = (uStandard == 0u) ? 0.060 : 0.075;
    float is_h_sync = 1.0 - step(h_sync_ratio * uEncW, x);
    
    float v_sync_top = (uStandard == 0u) ? 10.0 : 16.0;
    float v_sync_bot = (uStandard == 0u) ? 9.0 : 15.0;
    float is_v_sync = (1.0 - step(v_sync_top, y)) + step(uEncH - v_sync_bot, y);
    
    float sync_mask = max(is_h_sync, is_v_sync);

    if(uSyncEnabled == 1 && sync_mask > 0.0) {
        signal = uSyncLevel;
        chroma = 0.0; // Blank chroma during retrace
    }

    // Pack by connection type
    if(uConnType == 3u) {
        // VGA mode
        if(uScandouble == 1) {
            fragColor = vec4(sampleScanlineHann(uInputTex, vTexCoord.x, int(floor(y * 0.5)), vec2(uInW, uInH)), 1.0);
        } else {
            fragColor = vec4(textureHannRGB(uInputTex, vTexCoord, vec2(uInW, uInH)), 1.0);
        }
    } else if(uConnType == 2u) {
        // COMPONENT mode
        fragColor = vec4(rgb2ycbccr(sampleScanlineLinear(uInputTex, vTexCoord.x, line, vec2(uInW, uInH))), 1.0);
    } else {
        fragColor = vec4(signal, 0.0, 0.0, 1.0);
    }
}
