#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 WorldNormal;
out vec3 WorldPos;
out mat3 TBN;              // tangent-to-world matrix for normal mapping
out vec3 WorldTangent;     // for anisotropic specular
out vec3 WorldBitangent;   // for anisotropic specular

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;
uniform bool manualCulling;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    WorldPos = worldPos.xyz;

    // Normal matrix
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 worldNormal = normalize(normalMatrix * aNormal);
    Normal = worldNormal;
    WorldNormal = worldNormal;

    // TBN matrix: transform tangent-space normal to world space
    vec3 worldTangent = normalize(normalMatrix * aTangent);
    vec3 worldBitangent = normalize(normalMatrix * aBitangent);
    // Re-orthogonalize
    worldBitangent = normalize(cross(worldNormal, worldTangent));
    worldTangent = normalize(cross(worldNormal, worldBitangent));
    TBN = mat3(worldTangent, worldBitangent, worldNormal);

    // Pass world-space tangent frame for anisotropic shading
    WorldTangent = worldTangent;
    WorldBitangent = worldBitangent;

    // Manual backface culling
    if (manualCulling) {
        vec3 viewDir = normalize(viewPos - worldPos.xyz);
        float facing = dot(worldNormal, viewDir);
        if (facing < 0.0) {
            gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
            TexCoord = aTexCoord;
            return;
        }
    }

    TexCoord = aTexCoord;
    gl_Position = projection * view * worldPos;
}
