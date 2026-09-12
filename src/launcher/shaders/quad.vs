#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 avTexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = vec2(avTexCoord.x, avTexCoord.y);
    gl_Position = vec4(aPos, 0.0, 1.0);
}