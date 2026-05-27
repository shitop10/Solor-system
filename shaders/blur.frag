#version 330 core
// Gaussian blur (horizontal pass)
in vec2 vTexCoord;
uniform sampler2D image;
uniform float texelSize;  // 1.0 / textureWidth for horizontal
uniform float dirX;       // 1.0 for horizontal, 0.0 for vertical
uniform float dirY;       // 0.0 for horizontal, 1.0 for vertical
out vec4 FragColor;

// 9-tap Gaussian kernel weights (sigma ~ 2.0)
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 step = vec2(dirX, dirY) * texelSize;
    vec3 result = texture(image, vTexCoord).rgb * weights[0];
    for (int i = 1; i < 5; i++) {
        result += texture(image, vTexCoord + step * float(i)).rgb * weights[i];
        result += texture(image, vTexCoord - step * float(i)).rgb * weights[i];
    }
    FragColor = vec4(result, 1.0);
}
