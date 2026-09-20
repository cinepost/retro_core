#version 330 core

#pragma parameter bool uBypass "Bypass" 0 0 1

#pragma parameter float uSaturation "Saturation" 1.0 0.0 3.0
#pragma parameter float uFilterWidth "Filter Width" 2.25 0.01 10.0

in vec2 vTexCoord;
out vec4 fragColor;

uniform bool   uBypass;
uniform float  uSaturation;
uniform uint   uConnType;       // 0=RF, 1=Composite, 2=S-Video. 3=Component, 4=VGA
uniform uint   uStandard;       // 0=NTSC, 1=PAL
uniform int    uPALCombEnabled;
uniform int    uPALCombEdgeProtect;
uniform int    uSyncStripEnabled;
uniform float  uSyncLevel;
uniform int    uFrameCount;
uniform float  uInW, uInH;
uniform float  uOutW, uOutH;
uniform float  uEncW, uEncH;

uniform float  uFilterWidth; 

uniform sampler2D uEncTex;

#include "common.glsl"
#include "sampling.glsl"

vec3 xyz2rgb(vec3 c) {
    if(uStandard == NTSC) {
        // NTSC
        return YUV_to_RGB * c;
    }

    // PAL
    return YIQ_to_RGB * c;
}

void main() {
    float x = vTexCoord.x * uEncW;
    float y = vTexCoord.y * uEncH;
    int line = int(floor(y));

    // VGA mode / Bypass
    if(uConnType == VGA || uBypass) {
        fragColor = vec4(texture(uEncTex, vTexCoord).rgb, 1.0);
        return;
    }

    // COMPONENT MODE: Direct extraction, NO filtering/demodulation
    if(uConnType == COMPONENT) {
        vec3 component_signal = sampleScanlineLinear(uEncTex, vTexCoord.x, line, vec2(uInW, uInW)).rgb;
        component_signal.y *= uSaturation;
        component_signal.z *= uSaturation;
        fragColor = vec4(xyz2rgb(component_signal), 1.0);
        return;
    }

    // S-VIDEO / COMPOSITE / RF

    // Universal RF phase jitter cross-talk
    float rfNoise = 0.0;
    if (uConnType == RF) {
        // Combines a slow scrolling vertical wave (hum bar) with a faster high-frequency jitter
        float slowWave = sin(vTexCoord.y * 8.0 + float(uFrameCount) * 0.05) * 0.8;
        float fastJitter = sin(vTexCoord.y * 120.0 - float(uFrameCount) * 0.4) * 0.3;
        rfNoise = (slowWave + fastJitter) * 1.5;
    }

    float stepX = 0.9 / uEncW; // 0.9 is break samplint moire. TODO: think of a better way

    float totalY = 0.0;
    float totalI = 0.0;
    float totalQ = 0.0;
    
    float weight_sum_Y = 0.0;
    float weight_sum_I = 0.0;
    float weight_sum_Q = 0.0;

    // IMPORTANT
    // Doubling virtual subcarrier cycles per line effectively simulates the net result 
    // of phase cancellation per spatial frame footprint. It compresses the artifact 
    // frequency so it can be cleanly filtered out in a single static pass.
    float cyclesPerLine = 227.5 * 2.0; 
    float radiansPerPixel = (cyclesPerLine * 2.0 * 3.14159265) / uEncW;

    float scaleRatio = uEncW / uInW;

    float filterWidth = (uConnType == S_VIDEO) ? uFilterWidth * 0.85 : uFilterWidth;

    float filterWidthY = filterWidth * scaleRatio;
    float filterWidthI = filterWidth * scaleRatio * ((uConnType == S_VIDEO) ? 2.5 : 2.8);
    float filterWidthQ = filterWidth * scaleRatio * ((uConnType == S_VIDEO) ? 2.5 : 5.6);

    int filterTapRadius = int(ceil(filterWidthQ * 2.0));

    for (int i = -filterTapRadius; i <= filterTapRadius; i++) {
        vec2 sampleOffset = vec2(float(i) * stepX, 0.0);
        vec2 sampleTexCoord = vTexCoord + sampleOffset;
        
        /// 1. Read the raw modulated wave value from the Red channel
        vec2 signal = vec2(0.0);
        
        if(sampleTexCoord.x >= 0.001 && sampleTexCoord.x <= 0.999) {
            signal = sampleScanlineLinear(uEncTex, sampleTexCoord.x, line, vec2(uEncW, uEncH)).rg;
        }

        // 2. Reconstruct the phase angle analytically for the sampled position
        float sampleCurrentX = sampleTexCoord.x * uEncW;
        float angle = (sampleCurrentX * radiansPerPixel) + rfNoise;
        
        // 3. Demodulate components using the recovered local phase
        float rawY = (uConnType == S_VIDEO) ? signal.g : signal.r;

        if(uConnType == RF) {
            float shadow_luma = sampleScanlineLinear(uEncTex, sampleTexCoord.x - 0.0055, line, vec2(uEncW, uEncH)).r;
            rawY = mix(rawY, shadow_luma, -0.3);
        }

        float rawI = signal.r * sin(angle) * 2.0;
        float rawQ = signal.r * cos(angle) * 2.0;
        
        // 4. Evaluate Gaussian weights based on sample distance
        float dist = abs(float(i));
        float distSq = dist * dist;
        float wY = exp(-0.5 * distSq / (filterWidthY * filterWidthY));
        float wI = exp(-0.5 * distSq / (filterWidthI * filterWidthI));
        float wQ = exp(-0.5 * distSq / (filterWidthQ * filterWidthQ));
        
        // Accumulate filtered signals
        totalY += rawY * wY;
        weight_sum_Y += wY;
        
        totalI += rawI * wI;
        weight_sum_I += wI;

        totalQ += rawQ * wQ;
        weight_sum_Q += wQ;
    }

    vec3 decodedYIQ = vec3(
        totalY / weight_sum_Y,
        totalI / weight_sum_I,
        totalQ / weight_sum_Q
    );

    decodedYIQ.y *= uSaturation;
    decodedYIQ.z *= uSaturation;

    vec3 finalRGB = xyz2rgb(decodedYIQ);

    if(uConnType == RF) {
        finalRGB = finalRGB * 0.8 + vec3(0.09); 
    }

    fragColor = vec4(finalRGB, 1.0);
}
