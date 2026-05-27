#version 330 core
// Composite: blend bloom onto original scene
in vec2 vTexCoord;
uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomIntensity;
out vec4 FragColor;

void main() {
    vec3 scene = texture(sceneTex, vTexCoord).rgb;
    vec3 bloom = texture(bloomTex, vTexCoord).rgb;
    scene += bloom * bloomIntensity;
    // Simple tone mapping (Reinhard)
    scene = scene / (scene + vec3(1.0));
    // Gamma correction
    scene = pow(scene, vec3(1.0 / 2.2));
    FragColor = vec4(scene, 1.0);
}
