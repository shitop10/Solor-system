#version 330 core
in vec2 vPos;
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
