#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in uint vCharId[];
in vec4 vColor[]; // Incoming color array from vertex stage

out vec2 TexCoords;
out vec4 CharColor; // Emitted color to fragment shader

uniform bool uDoShadow;
uniform vec2 uScreenResolution; // vec2(1024.0, 288.0)
uniform vec2 uGlyphSize; // Size of a single letter relative to screen coordinates (e.g., vec2(0.05, 0.08))
uniform vec2 uAtlasGridSize;

void main() {
    uint charId = vCharId[0];
    vec4 charColor = vColor[0]; // Capture the point's unique color
    
    // Calculate atlas row and column (Assuming 16x16 grid atlas)
    float uStep = 1.0 / float(uAtlasGridSize.x);
    float vStep = 1.0 / float(uAtlasGridSize.y);
    float uStart = float(charId % uint(uAtlasGridSize.x)) * uStep;
    float vStart = float(charId / uint(uAtlasGridSize.y)) * vStep;

    vec2 delta = (uGlyphSize / uScreenResolution) * 2.0;
    vec4 basePos = gl_in[0].gl_Position;

    // Bottom-Left
    gl_Position = basePos;
    TexCoords = vec2(uStart, vStart);
    CharColor = charColor;
    EmitVertex();

    // Bottom-Right
    gl_Position = basePos + vec4(delta.x, 0.0, 0.0, 0.0);
    TexCoords = vec2(uStart + uStep, vStart);
    CharColor = charColor;
    EmitVertex();

    // Top-Left
    gl_Position = basePos + vec4(0.0, -delta.y, 0.0, 0.0);
    TexCoords = vec2(uStart, vStart + vStep);
    CharColor = charColor;
    EmitVertex();

    // Top-Right
    gl_Position = basePos + vec4(delta.x, -delta.y, 0.0, 0.0);
    TexCoords = vec2(uStart + uStep, vStart + vStep);
    CharColor = charColor;
    EmitVertex();

    EndPrimitive();
}
