#version 330 core

out vec4 FragColor;
in vec2 TexCoords;
in vec4 CharColor; // Receives the character's custom color quad mix

uniform sampler2D fontAtlas;
uniform vec3 uTintColor; // Pass vec3(0.0, 1.0, 0.0) for OSD green

void main() {
    // Sample mask from your font texture
    float mask = texture(fontAtlas, TexCoords).r;
    
    // Discard transparent texels completely so your underlying game isn't obscured
    if (mask < 0.01 || CharColor.a < 0.01) {
        discard;
    }
    
    FragColor = CharColor * vec4(uTintColor, 1.0);
}
