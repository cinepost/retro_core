#version 330 core

#pragma parameter bool uBypass "Bypass" 0 0 1
#pragma parameter float uNoiseLevel "Transmission Noise" 0.2 0.0 1.0

#pragma parameter float uSrcFilterWidth "Low-Pass Filter Width" 0.0 0.0 1.0
#pragma parameter float uSrcPeakScale "Low-Pass Peak" 5.0 1.0 10.0

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uInputTex;

uniform bool   uBypass;
uniform float  uRandom;
uniform float  uNoiseLevel;
uniform int    uFrameCount;

uniform float  uSrcFilterWidth;  // Standard deviation (sigma) of the Gaussian
uniform float  uSrcPeakScale;    // Scale factor for the central peak weight

uniform uint   uConnType;      // 0=RF, 1=Composite, 2=S-Video. 3=Component, 4=VGA
uniform uint   uStandard;      // 0=NTSC, 1=PAL
uniform int    uSyncEnabled;   // 1=inject sync pulses
uniform float  uSyncLevel;     // -0.3 to 0.0 (sync tip)
uniform int    uScandouble;    // Applies only to VGA/HDMI modes
uniform float  uInW, uInH;
uniform float  uOutW, uOutH;
uniform float  uEncW, uEncH;

#include "common.glsl"
#include "sampling.glsl"

vec3 rgb2xyz(vec3 c) {
    if(uStandard == NTSC) {
        // NTSC
        return RGB_to_YUV * c;
    }

    // PAL
    return RGB_to_YIQ * c;
}

void main() {
    float x = vTexCoord.x * uEncW;
    float y = vTexCoord.y * uEncH;
    int line = int(floor(y));

    if(uBypass) {
        fragColor.rgb = sampleScanlineNearest(uInputTex, vTexCoord.x, line, vec2(uInW, uInH)).rgb;
        return;
    } 

    float connection_noise_weight = uNoiseLevel * pow(float(5u - uConnType) / 5.0f, 2);

    if(uConnType == VGA) {
        // VGA mode
        if(uScandouble == 1) {
            fragColor = sampleScanlineHann(uInputTex, vTexCoord.x, int(floor(y * 0.5)), vec2(uInW, uInH));
            return;
        } else {
            fragColor = vec4(textureHannRGB(uInputTex, vTexCoord, vec2(uInW, uInH)), 1.0);
            return;
        }
    }

    // Pseudorandom high-frequency signal noise
    float signal_noise_random = (rand(vTexCoord + vec2(uRandom, -uRandom)) - 0.5);
    float signal_noise = 0.0f;

    if(uConnType == COMPONENT) {
        // COMPONENT mode
        float dynamicSeedU = float(uFrameCount) * 0.05; // dynamic seed for pseudorandom rolling noise 
        float dynamicSeedV = float(uFrameCount + uRandom) * 0.05; // dynamic seed for pseudorandom rolling noise 
        float chromaNoiseU = rand(vTexCoord * 0.1 + vec2(dynamicSeedU, -dynamicSeedU)) - 0.5;
        float chromaNoiseV = rand(vTexCoord * 0.2 + vec2(dynamicSeedV, -dynamicSeedV)) - 0.5;

        vec3 component_signal = rgb2xyz(sampleScanlineLinear(uInputTex, vTexCoord.x, line, vec2(uInW, uInH)).rgb);

        component_signal.x += signal_noise_random * connection_noise_weight;
        component_signal.y += chromaNoiseU * connection_noise_weight * 0.5;
        component_signal.z += chromaNoiseV * connection_noise_weight * 0.5;

        fragColor = vec4(component_signal, 1.0);
        return;
    }

    // S-VIDEO / COMPOSITE / RF
    float scaleRatioInv = uInW / uEncW;

    vec3 filteredRGB = vec3(0.0);
    float totalWeight = 0.0;

    float currentX = vTexCoord.x * uEncW;

    if(uSrcFilterWidth >= 0.01) { 
        float filterWidth = ceil(uSrcFilterWidth * 10);
        float srcStepX = 1.0 / uEncW;
        
        int steps = int(max(1.0, ceil(uSrcFilterWidth * 10)));

        for (int i = -steps; i <= steps; i++) {
            float offset = float(i) * srcStepX;
            float sampleCoord = vTexCoord.x + offset;

            // sampleScanlineHannSharp looks more like DAC
            vec3 rawRGB = sampleScanlineHannSharp(uInputTex, sampleCoord, line, vec2(uInW, uInH)).rgb;

            float dist = abs(float(i));
            float weight = exp(-0.5 * (dist * dist) / float(steps * steps));
            
            // Apply tweakable central peak scale modifier
            if (i == 0) {
                weight *= uSrcPeakScale;
            }
            
            filteredRGB += rawRGB * weight;
            totalWeight += weight;
        }
        filteredRGB /= totalWeight;//max(totalWeight, 0.0001);
    } else {
        filteredRGB = sampleScanlineHannSharp(uInputTex, vTexCoord.x, line, vec2(uInW, uInH)).rgb;
    }
    vec3 yiq = rgb2xyz(filteredRGB);

    // Universal RF phase jitter cross-talk
    float rfNoise = 0.0;
    if (uConnType == RF) {
        // Combines a slow scrolling vertical wave (hum bar) with a faster high-frequency jitter
        float slowWave = sin(vTexCoord.y * 8.0 + float(uFrameCount) * 0.05) * 0.8;
        float fastJitter = sin(vTexCoord.y * 120.0 - float(uFrameCount) * 0.4) * 0.3;
        rfNoise = (slowWave + fastJitter) * 1.51;
    }

    // IMPORTANT
    // Doubling virtual subcarrier cycles per line effectively simulates the net result 
    // of phase cancellation per spatial frame footprint. It compresses the artifact 
    // frequency so it can be cleanly filtered out in a single static pass.
    float cyclesPerLine = 227.5 * 2.0; 
    
    float radiansPerPixel = (cyclesPerLine * 2.0 * 3.14159265) / uEncW;

    float angle = (currentX * radiansPerPixel) + rfNoise;
    float chromaModulated = yiq.y * sin(angle) + yiq.z * cos(angle);
    float compositeSignal = yiq.x + chromaModulated;

    // Inject high-frequency white noise directly into the analog carrier wave
    float dynamicSeed = float(uFrameCount) * 0.05; // dynamic seed for pseudorandom rolling noise 
    float signal_noise_rolling = (rand(vTexCoord + vec2(dynamicSeed, -dynamicSeed)) - 0.5);
    signal_noise = signal_noise_random * 0.8 + signal_noise_rolling * 0.2;

    if(uConnType == S_VIDEO) {
        yiq.x += signal_noise * connection_noise_weight;
        chromaModulated += signal_noise * connection_noise_weight;
        fragColor = vec4(chromaModulated, yiq.x, 0.0, 1.0);
    } else {
        // COMPOSITE
        compositeSignal += signal_noise * connection_noise_weight;
        fragColor = vec4(compositeSignal, 0.0, 0.0, 1.0);
    }
}
