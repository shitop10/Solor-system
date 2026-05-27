#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aBrightness;
layout(location = 2) in float aColorIdx;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

out float vBrightness;
out float vColorIdx;
out float vTime;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);

    // Twinkling effect: slow + medium + fast components
    float twinkle = 0.85
        + 0.08 * sin(time * 3.0 + aPos.x * 100.0 + aPos.y * 73.0)
        + 0.05 * sin(time * 7.5 + aPos.z * 150.0 + aPos.x * 50.0)
        + 0.02 * sin(time * 15.0 + aPos.y * 200.0);
    vBrightness = aBrightness * twinkle;
    vColorIdx = aColorIdx;
    vTime = time;

    // Point size tiers for brighter stars
    if (aBrightness > 1.5)
        gl_PointSize = 4.5;
    else if (aBrightness > 0.8)
        gl_PointSize = 3.2;
    else if (aBrightness > 0.4)
        gl_PointSize = 2.2;
    else
        gl_PointSize = 1.5;
}
