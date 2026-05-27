#version 330 core
in float vBrightness;
in float vColorIdx;
in float vTime;
out vec4 FragColor;

void main()
{
    // Star color categories
    vec3 coolWhite  = vec3(0.90, 0.94, 1.00);
    vec3 warmYellow = vec3(1.00, 0.92, 0.70);
    vec3 blueWhite  = vec3(0.65, 0.75, 1.00);

    // Mix based on color index
    float t = vColorIdx;
    vec3 baseColor;
    if (t < 0.5)
        baseColor = mix(coolWhite, warmYellow, t * 2.0);
    else
        baseColor = mix(warmYellow, blueWhite, (t - 0.5) * 2.0);

    // Brightness boost
    float b = clamp(vBrightness, 0.0, 4.0);

    // Soft circular glow for point sprites
    vec2 center = gl_PointCoord - vec2(0.5);
    float dist = length(center);
    // Sharper core, softer falloff
    float core = 1.0 - smoothstep(0.0, 0.15, dist);
    float halo = 1.0 - smoothstep(0.0, 0.5, dist);
    float alpha = core * 0.9 + halo * 0.1;
    alpha = pow(alpha, 1.2);

    vec3 color = baseColor * b * (0.85 + halo * 0.15);
    FragColor = vec4(color, alpha);
}
