#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
uniform vec3 uViewPos;
uniform vec3 uGlowColor;
uniform float uIntensity;
out vec4 FragColor;

void main()
{
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 norm = normalize(vNormal);

    // Fresnel: glow is brightest at edges, soft in center
    float fresnel = 1.0 - abs(dot(norm, viewDir));
    fresnel = pow(fresnel, 2.5);

    // Multi-component glow: edge + halo
    float edgeGlow = fresnel * 0.55;
    float haloGlow = pow(fresnel, 0.6) * 0.20;

    float alpha = (0.05 + edgeGlow + haloGlow) * uIntensity;
    vec3 color = uGlowColor * (0.25 + fresnel * 0.75 + haloGlow * 0.3);

    FragColor = vec4(color * uIntensity, clamp(alpha, 0.0, 1.0));
}
