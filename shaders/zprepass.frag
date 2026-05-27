#version 330 core
// Z-Prepass: depth-only pass. Writes depth via gl_FragDepth
// to demonstrate manual Z-Buffer semantics.
out vec4 FragColor;

void main()
{
    // gl_FragCoord is a built-in — no need to declare it.
    // Explicitly set depth for educational Z-Buffer demo.
    gl_FragDepth = gl_FragCoord.z;
    // Color is not written (masked by glColorMask), but set to 0.
    FragColor = vec4(0.0);
}
