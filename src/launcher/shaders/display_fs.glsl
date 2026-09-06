#version 330 core

#pragma parameter vec3 uTintColor "Tint Color" 1.0 1.0 1.0  0.0 0.0 0.0  1.0 1.0 1.0
#pragma parameter vec2 uPicShift "Picture Shift" 0.0 0.0  -1.0 -1.0  1.0 1.0

#pragma parameter vec2 uPicStretch "Picture Stretch" 1.0 1.0  0.01 0.01  2.0 2.0
#pragma parameter float uScanFlicker "Flickering" 0.25  0.0  1.0

in vec2 vTexCoord;
out vec4 fragColor;

uniform int     uMode; // 0 = RF, 1 = Composite, 2 = Component, 3 = VGA
uniform vec2    uDecodedResolution;
uniform vec2    uOutResolution;

uniform vec3    uTintColor;
uniform vec2    uPicShift;
uniform vec2    uPicStretch;
uniform float   uScanFlicker;

uniform float   uEdgeDefocus;
uniform vec2    uMisalignment;
uniform float   uAberration;

uniform int     uFrameCount;
uniform sampler2D uDecodedTexture;

#include "common.glsl"
#include "sampling.glsl"

// Hardness of scanline.
//  -8.0 = soft
// -16.0 = medium
float hardScan = -16.0;

// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
float hardPix = -4.0;

// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
vec2 warp=vec2(1.0/128.0, 1.0/128.0); 

// Amount of shadow mask.
float maskDark = 0.35;
float maskLight = 1.1;

// Falloff shape.
// 1.0  = exp(x)
// 1.25 = in between
// 2.0  = gaussian
// 3.0  = more square
float shape = 3.0;

// Amp signal.
float overdrive = 1.95;

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
vec3 ToLinear(vec3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
vec3 ToSrgb(vec3 c){return vec3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}

// Set to zero, or remove Test() if using this shader.
#if 1
vec3 Test(vec3 c){
    return clamp(c + c*c, vec3(0.005), vec3(1.0));
    return c;
}
#else
vec3 Test(vec3 c){return c;}
#endif

// Distance in emulated pixels to nearest texel.
vec2 Dist(vec2 pos){
    pos = pos * uDecodedResolution;
    return -((pos-floor(pos))-vec2(0.5));
}

// Try different filter kernels.
float Gaus(float pos,float scale){
    return exp2(scale*pow(abs(pos), shape));
}

// Return scanline weight.
float Scan(vec2 pos, float off, float mul = 1.0) {
    float dst = Dist(pos).y * uPicStretch.y;
    return Gaus(dst + off * uPicStretch.y, hardScan / mul);
}

// Barrel distortion of scanlines, and end of screen alpha.
vec2 barrelDistortion(vec2 uv) {
    uv = uv * 2.0 - 1.0;    
    uv *= vec2(1.0 + (uv.y * uv.y) * warp.x, 1.0 + (uv.x * uv.x) * warp.y);
    return uv * 0.5 + 0.5;
}

/**
 * Calculates the non-linear electron beam energy from an RGB color.
 */
float calculateBeamEnergy(vec3 rgb) {
    // 1. Convert to perceived grayscale luminance (Rec. 709 linear coefficients)
    float luma = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    
    // 2. Approximate physical beam current curve.
    // In hardware, current grows exponentially with brightness.
    // A power of 1.5 to 2.0 beautifully mimics flyback transformer sag.
    float beamCurrent = pow(luma, 1.5);
    
    return clamp(beamCurrent, 0.25, 1.0);
}

// Allow nearest scan lines to effect pixel (slight overlap effect).
vec3 Tri(vec2 uv) {
    vec2 warp_uv = barrelDistortion(uv);

    warp_uv += uPicShift;
    warp_uv.y = ((warp_uv.y - 0.5) / uPicStretch.y) + 0.5;

    float line = floor(warp_uv.y * uDecodedResolution.y);
    float tex_u = ((warp_uv.x - 0.5) / uPicStretch.x) + 0.5;

    vec3 sampleA = sampleScanlineLinear(uDecodedTexture, tex_u, int(line-1), uDecodedResolution);
    vec3 sampleB = sampleScanlineLinear(uDecodedTexture, tex_u, int(line), uDecodedResolution);
    vec3 sampleC = sampleScanlineLinear(uDecodedTexture, tex_u, int(line+1), uDecodedResolution);

    float beamEnergyA = calculateBeamEnergy(sampleA);    
    float beamEnergyB = calculateBeamEnergy(sampleB);
    float beamEnergyC = calculateBeamEnergy(sampleC);

    vec3 a=Test(ToLinear(sampleA));
    vec3 b=Test(ToLinear(sampleB));
    vec3 c=Test(ToLinear(sampleC));

    vec2 scan_uv = warp_uv;
    float scan_off = uScanFlicker * (uFrameCount % 2) / 2.0;

    float wa=Scan(scan_uv,-1.0 + scan_off, beamEnergyA);
    float wb=Scan(scan_uv, 0.0 + scan_off, beamEnergyB);
    float wc=Scan(scan_uv, 1.0 + scan_off, beamEnergyC);

    return (a*wa+b*wb+c*wc) * overdrive;

// Slower and no visual difference
/* 
    vec3 a=Horz3(warp_uv,-2.0);
    vec3 b=Horz5(warp_uv,-1.0);
    vec3 c=Horz7(warp_uv, 0.0);
    vec3 d=Horz5(warp_uv, 1.0);
    vec3 e=Horz3(warp_uv, 2.0);
    float wa=Scan(warp_uv,-2.0);
    float wb=Scan(warp_uv,-1.0);
    float wc=Scan(warp_uv, 0.0);
    float wd=Scan(warp_uv, 1.0);
    float we=Scan(warp_uv, 2.0);
    return (a*wa+b*wb+c*wc+d*wd+e*we)*overdrive;
*/
}

vec3 Mask(vec2 uv) {
    uv.x = fract(uv.x/3.0);
    vec3 mask = vec3(maskDark, maskDark, maskDark);
    if(uv.x<0.333) {
        mask.r = maskLight;
    } else if(uv.x<0.666) {
        mask.g = maskLight;
    } else {
        mask.b = maskLight;
    }
    return mask;
}        

void main() {
    vec2 uv = vTexCoord;

    float edgeDefocus = 0.033; uEdgeDefocus;
    float aberration = 0.002;
    vec2 misalignment = vec2(0.0005, -0.0005);//vec2(-0.00085, 0.00025);
    
    vec3 color = vec3(0.0);

    if(misalignment.x > 0.00001 || misalignment.y > 0.00001) {
        color.r = Tri(uv - misalignment).r;
        color.g = Tri(uv).g;
        color.b = Tri(uv + misalignment).b;
    } else {
        color = Tri(uv);
    }

    if(edgeDefocus > 0.00001 || aberration > 0.00001) {
        vec2 centeredUV = (uv - 0.5) * 2.0;
        float distFromCenter = length(centeredUV);
        float distSq = distFromCenter * distFromCenter;

        vec2 toSample = uv - vec2(0.5);
        vec2 radialDir = (distFromCenter > 0.00001) ? toSample / distFromCenter : vec2(0.0);

        float splitAmount = distSq * aberration;

        float blurR = distSq * edgeDefocus * 0.024; // Medium
        float blurG = distSq * edgeDefocus * 0.016; // Sharpest
        float blurB = distSq * edgeDefocus * 0.032; // Blurriest

        vec2 uvR = uv - misalignment - (radialDir * splitAmount);
        vec2 uvG = uv;
        vec2 uvB = uv + misalignment + (radialDir * splitAmount);

        vec2 rStepH = vec2(0.866, 0.5) * blurR;
        vec2 rStepV = vec2(-0.5, 0.866) * blurR;

        float colorR = Tri(uvR + rStepH).r;
        colorR += Tri(uvR - rStepH).r;
        colorR += Tri(uvR + rStepV).r;
        colorR += Tri(uvR - rStepV).r;
        colorR *= 0.25;

        float colorG = Tri(uvG + vec2(blurG, 0.0)).g;
        colorG += Tri(uvG - vec2(blurG, 0.0)).g;
        colorG += Tri(uvG + vec2(0.0, blurG)).g;
        colorG += Tri(uvG - vec2(0.0, blurG)).g;
        colorG *= 0.25;

        vec2 bStepH = vec2(0.5, 0.866) * blurB;
        vec2 bStepV = vec2(-0.866, 0.5) * blurB;

        float colorB = Tri(uvB + bStepH).b;
        colorB += Tri(uvB - bStepH).b;
        colorB += Tri(uvB + bStepV).b;
        colorB += Tri(uvB - bStepV).b;
        colorB *= 0.25;

        color = color + vec3(colorR, colorG, colorB);
        color *= 0.5;
    }

    fragColor.rgb = color * uTintColor * Mask(vTexCoord.xy * uOutResolution);
    fragColor.a = 1.0;  
    fragColor.rgb = ToSrgb(fragColor.rgb) * vignette(vTexCoord, 3.0) * 1.1;

    //fragColor.rgb = texture(uDecodedTexture, vTexCoord).rgb;
}