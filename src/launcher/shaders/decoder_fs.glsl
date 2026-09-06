#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uEncTex;
uniform uint   uConnType;
uniform uint   uStandard;      // 0=NTSC, 1=PAL
uniform int    uPALCombEnabled;
uniform int    uPALCombEdgeProtect;
uniform int    uSyncStripEnabled;
uniform float  uSyncLevel;
uniform float  uInW, uInH;
uniform float  uOutW, uOutH;
uniform float  uEncW, uEncH;

#include "sampling.glsl"

vec3 ycbcr2rgb(vec3 c) {
    float Y = c.r;
    float Cb = c.g;
    float Cr = c.b;
    return vec3(
        Y + 1.402 * Cr,
        Y - 0.344136 * Cb - 0.714136 * Cr,
        Y + 1.772 * Cb
    );
}

void main() {
    float x = vTexCoord.x * uEncW;
    float y = vTexCoord.y * uEncH;
    int line = int(floor(y));

    // VGA mode
    if(uConnType == 3u) {
        fragColor = vec4(texture(uEncTex, vTexCoord).rgb, 1.0);
        return;
    }

    // COMPONENT MODE: Direct extraction, NO filtering/demodulation
    if(uConnType == 2u) {
        fragColor = vec4(ycbcr2rgb(sampleScanlineLinear(uEncTex, vTexCoord.x, line, vec2(uInW, uInW))), 1.0);
        return;
    }


    float is_odd = mod(line, 2.0) > 0.5 ? 1.0 : 0.0;
    //float signal = texture(uEncTex, vTexCoord).r;
    float signal = sampleScanlineLinear(uEncTex, vTexCoord.x, line, vec2(uEncW, uEncH)).r;

    // Sync stripping (fixed step syntax)
    if(uSyncStripEnabled == 1) {
        float h_sync_ratio = (uStandard == 0u) ? 0.060 : 0.075;
        float is_h_sync = 1.0 - step(h_sync_ratio * uEncW, x);
        float v_sync_top = (uStandard == 0u) ? 10.0 : 16.0;
        float v_sync_bot = (uStandard == 0u) ? 9.0 : 15.0;
        float is_v_sync = (1.0 - step(v_sync_top, y)) + step(uEncH - v_sync_bot, y);
        float sync_mask = max(is_h_sync, is_v_sync);
        signal = mix(signal, 0.075, sync_mask);
    }

    // Phase calculation (MUST match encoder exactly)
    // NOTE: Swap ternary operators if your C++ code maps 0=NTSC, 1=PAL
    float cycles_per_line = (uStandard == 0u ? 227.5 : 283.752);

    cycles_per_line *= 1;

    float phase_base = 2.0 * 3.14159265358979 * cycles_per_line * vTexCoord.x;
    float line_offset = (uStandard == 1u) ? mod(line, 2.0) * 3.14159265358979 : 0.0;

    // Carrier period in pixels
    float period = uEncW / cycles_per_line;

    // LPF widths scaled to carrier period (FIXES BLUR & STRIPES)
    // NTSC: 4.2 MHz luma, 1.3 MHz chroma
    // PAL:  5.0 MHz luma, 1.5 MHz chroma
    float sigmaY = period * (uStandard == 0u ? 1.7 : 1.4);
    float sigmaC = period * 0.45; // Strictly removes 2*f_sc carrier beat

    // 1. Extract luma with tight low-pass
    float filtY = 0.0, wY = 0.0;
    for(int i = -14; i <= 14; i++) {
        float dx = float(i) / uEncW;
        vec2 uv = clamp(vTexCoord + vec2(dx, 0.0), vec2(0.0), vec2(1.0));
        float w = exp(-0.5 * (dx * uEncW / sigmaY) * (dx * uEncW / sigmaY));
        
        filtY += sampleScanlineLinear(uEncTex, uv.x, line, vec2(uEncW, uEncH)).r * w;

        wY += w;
    }
    filtY /= wY;

    // 2. Demodulate chroma from residual (prevents luma/chroma bleed)
    float residual = signal - filtY;
    float filtI = 0.0, filtQ = 0.0, wSum = 0.0;
    
    // Extended taps to properly roll off 2*f_sc
    for(int i = -8; i <= 8; i++) {
        float dx = float(i) / uEncW;
        vec2 uv = clamp(vTexCoord + vec2(dx, 0.0), vec2(0.0), vec2(1.0));

        float s = sampleScanlineLinear(uEncTex, uv.x, line, vec2(uEncW, uEncH)).r;

        // Phase tracks continuously across filter taps
        float phase_tap = phase_base + 2.0 * 3.14159265358979 * cycles_per_line * dx + line_offset;
        
        float wC = exp(-0.5 * (dx * uEncW / sigmaC) * (dx * uEncW / sigmaC));
        filtI += (s * sin(phase_tap)) * wC;
        filtQ += (s * cos(phase_tap)) * wC;
        wSum += wC;
    }
    filtI /= wSum; filtQ /= wSum;

    // PAL sign correction (compensate encoder alternation)
    if(uStandard == 1u) {
        //filtI *= (is_odd > 0.5 ? -1.0 : 1.0);
        //filtQ *= (is_odd > 0.5 ? -1.0 : 1.0);
    }

    // Scale to chroma amplitude
    float Cb = filtI * 2.0;
    float Cr = filtQ * 2.0;
    float Y = filtY;

    // PAL delay-line comb filter (phase-aware)
    if(uPALCombEnabled == 1 && uStandard == 1u) {
        float y_prev = line - 1.0;
        vec3 yc_prev = texture(uEncTex, vec2(vTexCoord.x, y_prev/uEncH)).rgb;
        
        float phase_prev = phase_base + 2.0 * 3.14159265358979 * cycles_per_line * 0.0 + mod(y_prev, 2.0) * 3.14159265358979;
        float Cb_prev = yc_prev.g * sin(phase_prev) * 2.0;
        float Cr_prev = yc_prev.b * cos(phase_prev) * 2.0;
        if(mod(y_prev, 2.0) > 0.5) { Cb_prev = -Cb_prev; Cr_prev = -Cr_prev; }

        float blend = uPALCombEdgeProtect == 1 ? 0.6 : 1.0;
        Cb = mix(Cb, (Cb + Cb_prev) * 0.5, blend);
        Cr = mix(Cr, (Cr + Cr_prev) * 0.5, blend);
        Y = mix(Y, (Y + yc_prev.r) * 0.5, blend * 0.25);
    }

    // Clamp to video ranges
    Cb = clamp(Cb, -0.5, 0.5);
    Cr = clamp(Cr, -0.5, 0.5);
    Y = clamp(Y, 0.0, 1.0);

    vec3 decodedRGB = ycbcr2rgb(vec3(Y, Cb, Cr));

    // Apply the NTSC phase drift / greenDrift rotation matrix here
    if(uStandard == 1u) {
        float greenDrift = 0.05;
        decodedRGB.g += decodedRGB.r * greenDrift;
        decodedRGB.r -= decodedRGB.g * (greenDrift * 0.5);
    }

    fragColor = vec4(decodedRGB, 1.0);
}
