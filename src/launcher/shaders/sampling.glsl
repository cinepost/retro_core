// Optimized hanning texture RGB channel sampling
vec3 textureHannRGB(sampler2D tex, vec2 uv, vec2 texSize) {
    vec2 texelCoord = uv * texSize - 0.5;
    vec2 f = fract(texelCoord);
    ivec2 i = ivec2(floor(texelCoord)); // Native integer vector for texelFetch
    
    const float PI = 3.14159265359;
    vec2 w0 = 0.5 + 0.5 * cos(PI * f);
    
    vec3 texel00 = texelFetch(tex, i + ivec2(0, 0), 0).rgb;
    vec3 texel10 = texelFetch(tex, i + ivec2(1, 0), 0).rgb;
    vec3 texel01 = texelFetch(tex, i + ivec2(0, 1), 0).rgb;
    vec3 texel11 = texelFetch(tex, i + ivec2(1, 1), 0).rgb;
    
    vec3 row0 = mix(texel10, texel00, w0.x); // Horizontal blend (top row)
    vec3 row1 = mix(texel11, texel01, w0.x); // Horizontal blend (bottom row)
    
    return mix(row1, row0, w0.y); // Vertical blend
}

// Horizontally linear, vertically nearest-neighbor sampling
vec3 textureLinearXNearestY_RGB(sampler2D tex, vec2 uv, vec2 texSize) {
    vec2 texelCoord = uv * texSize - 0.5;
    float fx = fract(texelCoord.x);
    ivec2 i = ivec2(floor(texelCoord)); // i.y is now perfectly row-aligned
    
    vec3 texel0 = texelFetch(tex, i + ivec2(0, 0), 0).rgb; // Left texel
    vec3 texel1 = texelFetch(tex, i + ivec2(1, 0), 0).rgb; // Right texel
    
    return mix(texel0, texel1, fx);
}

// 1D Horizontal Linear interpolation along scanline rowN
vec3 sampleScanlineLinear(sampler2D tex, float u, int rowN, vec2 texSize) {
    float texelX = u * texSize.x - 0.5;
    
    int iX = int(floor(texelX));
    float fx = fract(texelX);
    vec3 texelLeft  = texelFetch(tex, ivec2(iX,     rowN), 0).rgb;
    vec3 texelRight = texelFetch(tex, ivec2(iX + 1, rowN), 0).rgb;
    
    return mix(texelLeft, texelRight, fx);
}

// 1D Horizontal Hann window interpolation along scanline rowN
vec3 sampleScanlineHann(sampler2D tex, float u, int rowN, vec2 texSize) {
    float texelX = u * texSize.x - 0.5;
    
    int iX = int(floor(texelX));
    float fx = fract(texelX);
    
    const float PI = 3.14159265359;
    float w0 = 0.5 + 0.5 * cos(PI * fx);
    
    vec3 texelLeft  = texelFetch(tex, ivec2(iX,     rowN), 0).rgb;
    vec3 texelRight = texelFetch(tex, ivec2(iX + 1, rowN), 0).rgb;
    
    return mix(texelRight, texelLeft, w0);
}

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
vec3 sampleFetchWithOffset(sampler2D tex, vec2 uv, vec2 off, vec2 texSize){
    vec2 uvOffset = off / texSize;
    vec2 targetUV = uv + uvOffset;
    vec2 centeredUV = (floor(targetUV * texSize) + 0.5) / texSize;

    if(centeredUV.x <= 0.0 || centeredUV.x >= 1.0 || centeredUV.y <= 0.0 || centeredUV.y >= 1.0) {
        return vec3(0.0, 0.0, 0.0);
    }

    return texture(tex, centeredUV).rgb;
}