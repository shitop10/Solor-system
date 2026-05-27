#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec4 ringColor;
uniform float alpha;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float fogDensity;
uniform vec3 fogColor;
uniform bool fogEnabled;
uniform int fogMode;
uniform float fogNear;
uniform float fogFar;

void main()
{
    float u = TexCoord.x; // 0=inner edge, 1=outer edge

    // ── Multi-band Saturn ring structure ──────────────────
    // C-ring | gap | B-ring (brightest) | Cassini division | A-ring
    float bandAlpha = 1.0;
    float bright = 0.0;
    vec3 bandColor = ringColor.rgb;

    if (u < 0.15) {
        // C ring — faint inner, slightly bluish
        bright = 0.50 + sin(u * 50.0) * 0.20 + sin(u * 90.0) * 0.15;
        bandAlpha = 0.55;
        bandColor = ringColor.rgb * vec3(0.85, 0.85, 0.95);
    } else if (u < 0.28) {
        // Gap between C and B — partial transparency
        bright = 0.25 + sin(u * 120.0) * 0.15;
        bandAlpha = 0.30;
    } else if (u < 0.55) {
        // B ring — brightest band
        bright = 0.85 + sin(u * 70.0) * 0.35 + sin(u * 140.0+2.0) * 0.20
               + sin(u * 200.0+4.0) * 0.12 + sin(u * 280.0+1.0) * 0.08;
        bandAlpha = 0.92;
        bandColor = ringColor.rgb * vec3(1.05, 1.02, 0.95);
    } else if (u < 0.63) {
        // Cassini division — dark gap
        bright = 0.12 + sin(u * 250.0) * 0.08;
        bandAlpha = 0.15;
    } else {
        // A ring — bright outer
        bright = 0.75 + sin(u * 65.0) * 0.32 + sin(u * 120.0+3.0) * 0.20
               + sin(u * 180.0+5.0) * 0.12;
        bandAlpha = 0.82;
        bandColor = ringColor.rgb * vec3(1.02, 0.98, 0.90);
    }

    // Radial density variation (subtle ringlets)
    float microDetail = sin(u * 350.0) * 0.06 + sin(u * 520.0+3.0) * 0.04;
    bright += microDetail;

    // ── Lighting (Lambertian with ambient) ────────────────
    vec3 N = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float NdotL = max(dot(N, lightDir), 0.35);
    // Add some back-scatter for rings (they're translucent ice particles)
    float backScatter = max(dot(N, -lightDir), 0.0) * 0.15;
    float lighting = NdotL + backScatter;

    vec3 finalColor = bandColor * bright * lighting;

    // ── Fog ───────────────────────────────────────────────
    if (fogEnabled) {
        float d = length(viewPos - FragPos);
        float ff = 1.0;
        if (fogMode == 0)
            ff = (fogFar - d) / (fogFar - fogNear);
        else if (fogMode == 1)
            ff = exp(-fogDensity * d);
        else if (fogMode == 2)
            ff = exp(-fogDensity * d * d);
        else {
            float hf = exp(-abs(FragPos.y) * 0.01);
            ff = exp(-fogDensity * d * hf);
        }
        ff = clamp(ff, 0.0, 1.0);
        float horizon = abs(normalize(viewPos - FragPos).y);
        vec3 fCol = mix(fogColor * 0.5, fogColor * 1.8, smoothstep(0.0, 0.3, horizon));
        finalColor = mix(fCol, finalColor, ff);
    }

    FragColor = vec4(finalColor, alpha * bandAlpha);
}
