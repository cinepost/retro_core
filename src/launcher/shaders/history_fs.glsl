    #version 330 core
    in vec2 vTexCoord;
    out vec4 FragColor;
    uniform vec3 uDecayCoefficients;
    uniform sampler2D uDecodedTexture;
    uniform sampler2D uHistoryTexture;

    // Typical CRT gamma is roughly 2.5
    const float CRT_GAMMA = 2.5;

    void main() {
        vec3 currentGamma = texture(uDecodedTexture, vec2(vTexCoord.x, 1.0 - vTexCoord.y)).rgb;
        vec3 historyGamma = texture(uHistoryTexture, vec2(vTexCoord.x, vTexCoord.y)).rgb;

        // Convert both to LINEAR space (physics space)
        vec3 currentLinear = pow(currentGamma, vec3(CRT_GAMMA));
        vec3 historyLinear = pow(historyGamma, vec3(CRT_GAMMA));

        // Apply phosphor decay coefficients using the max() strategy
        vec3 mixedLinear = max(currentLinear, historyLinear * uDecayCoefficients);

        // Convert back to Gamma space for the next pass
        vec3 accumulated = pow(mixedLinear, vec3(1.0 / CRT_GAMMA));
        FragColor = vec4(accumulated, 1.0);
    }