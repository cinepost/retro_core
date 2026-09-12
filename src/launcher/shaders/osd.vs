#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in uint aCharId;
layout (location = 2) in vec4 aColor;

out uint vCharId;
out vec4 vColor;

uniform vec2 uScreenResolution;

void main() {
    float ndcX = (aPos.x / uScreenResolution.x) * 2.0 - 1.0;
    float ndcY = 1.0 - (aPos.y / uScreenResolution.y) * 2.0;

    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
    vCharId = aCharId;
    vColor = aColor;
}