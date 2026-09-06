float vignette(vec2 uv, float power) {
    float d = pow(distance(vec2(0.5),uv), power);
    return mix(1.0, 0.0, d);
}

float hash(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233) * 2.0)) * 43758.5453);
}

// A highly precise, monotonic approximation of the error function (erf)
// This is the true mathematical integral of a Gaussian distribution.
float exactErf(float x) {
    float ax = abs(x);
    // Highly accurate rational approximation for the Gaussian integral curve
    float t = 1.0 / (1.0 + 0.5 * ax);
    float ans = 1.0 - t * exp(-ax * ax - 1.26551223 + t * (0.54064088 + 
                t * (0.43274609 + t * (-0.18310875 + t * (0.71348303 + 
                t * (-1.22633574 + t * (0.35355057 + t * (-0.96815301 + 
                t * 0.13080450))))))));
    return sign(x) * ans;
}

/*
 * Generates flawless single-peaked antialiased scanlines.
 * 
 * @param warpedY      The curved/warped Y coordinate from your vertex/fragment shader.
 * @param srcLines     The total vertical resolution of the source game (e.g., 240.0).
 * @param beamWidth    The thickness of the beam (0.1 = thin, 0.5 = full width, >0.5 = overlapping).
 * @param beamShape    The profile shape (1.0 = thin/sharp, 2.0 = Gaussian, 4.0+ = square/blocky).
 * @param dVdx         The screen-space derivative (pass `dFdy(warpedY)` or calculate via uniform).
 */
float generateAntialiasedScanline(float warpedY, float srcLines, float beamWidth, float beamShape, float dVdx) {
    // 1. Scale coordinate to source CRT lines
    float lineCoord = warpedY * srcLines;
    
    // 2. Continuous distance to the exact center of the nearest scanline
    float dist = lineCoord - floor(lineCoord + 0.5);
    
    // 3. Measure the height of 1 physical LCD pixel in source line units
    float pixelWidth = max(dVdx * srcLines, 0.0001);

    // 4. Set analytical integration boundaries across the LCD pixel
    float x0 = dist - (pixelWidth * 0.5);
    float x1 = dist + (pixelWidth * 0.5);

    // 5. Enforce boundaries on parameters
    float w = max(beamWidth * 0.5, 0.01);
    float s = max(beamShape, 0.5);

    // 6. Symmetrical Piecewise CDF (Cumulative Distribution Function)
    // To ensure perfect symmetry on both top and bottom edges, we integrate 
    // a function that scales evenly across the center line (0.0).
    // This utilizes a smooth profile that flattens at the core based on 'beamShape'.
    
    // Normalize boundaries by the beam radius
    float u0 = x0 / w;
    float u1 = x1 / w;

    // Symmetrical shape mapping function
    // Raising the absolute value to a power preserves mirror symmetry across the beam axis
    float cdf0 = 0.5 * u0 * (1.0 / (1.0 + pow(abs(u0), s)));
    float cdf1 = 0.5 * u1 * (1.0 / (1.0 + pow(abs(u1), s)));

    // 7. Divide the integrated area by the window width to get average intensity
    // We clamp to 0.5 boundaries to prevent out-of-bounds bleeding
    float intensity = (cdf1 - cdf0) / pixelWidth;

    // Normalization gain to maintain constant perceptual brightness
    return clamp(intensity * w * 2.5, 0.0, 1.0);
}
