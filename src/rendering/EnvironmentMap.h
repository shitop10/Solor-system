#pragma once
#include <glad/glad.h>
#include "MglMath.h"
#include <vector>
#include <cmath>
#include <cstdio>

// ============================================================
//  Environment Map — procedural starfield cubemap
//  Used for PBR environment reflections
// ============================================================
class EnvironmentMap {
public:
    unsigned int cubemapID = 0;
    int faceSize = 256;

    bool enabled = true;
    float intensity = 0.5f;  // reflection strength multiplier

    void init(int size = 256) {
        faceSize = size;
        cleanup();

        glGenTextures(1, &cubemapID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        // Generate 6 faces programmatically
        for (int i = 0; i < 6; i++) {
            std::vector<unsigned char> data = generateFace(i, size);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        printf("[EnvMap] Procedural starfield cubemap %dx%d created\n", size, size);
    }

    void bind(int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
    }

    void cleanup() {
        if (cubemapID) { glDeleteTextures(1, &cubemapID); cubemapID = 0; }
    }

private:
    // Hash functions for procedural star generation
    static float hash(float x, float y, float z) {
        float h = x * 374761393.0f + y * 668265263.0f + z * 1442968193.0f;
        h = sin(h) * 43758.5453f;
        return h - floor(h);
    }

    static float hash2D(float x, float y) {
        float h = x * 127.1f + y * 311.7f;
        h = sin(h) * 43758.5453f;
        return h - floor(h);
    }

    // Generate one cubemap face with a procedural starfield + nebula
    std::vector<unsigned char> generateFace(int faceIndex, int size) {
        std::vector<unsigned char> data(size * size * 3, 0);

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // UV in [-1, 1]
                float u = (float)x / size * 2.0f - 1.0f;
                float v = (float)y / size * 2.0f - 1.0f;

                // 3D direction based on face
                glm::vec3 dir;
                switch (faceIndex) {
                    case 0: dir = glm::vec3( 1.0f, -v, -u); break;  // +X
                    case 1: dir = glm::vec3(-1.0f, -v,  u); break;  // -X
                    case 2: dir = glm::vec3( u,  1.0f,  v); break;  // +Y
                    case 3: dir = glm::vec3( u, -1.0f, -v); break;  // -Y
                    case 4: dir = glm::vec3( u, -v,  1.0f); break;  // +Z
                    case 5: dir = glm::vec3(-u, -v, -1.0f); break;  // -Z
                }
                dir = glm::normalize(dir);

                // Base dark space color
                float r = 0.01f, g = 0.01f, b = 0.03f;

                // Nebula: large-scale color variation
                float nebula = hash2D(dir.x * 3.0f + dir.y * 1.7f, dir.z * 2.3f) * 0.6f
                             + hash2D(dir.y * 4.1f + dir.z * 1.3f, dir.x * 3.7f) * 0.4f;
                nebula = nebula * nebula;  // sharpen

                if (nebula > 0.55f) {
                    float n = (nebula - 0.55f) / 0.45f;
                    // Blue-purple nebula regions
                    r += n * 0.08f;
                    g += n * 0.04f;
                    b += n * 0.15f;
                }

                // Stars: random bright points
                float starRNG = hash(dir.x * 1973.0f, dir.y * 3527.0f, dir.z * 5179.0f);
                float starBright = 0.0f;

                // Multiple octaves for different star sizes
                float star1 = (starRNG > 0.998f) ? (starRNG - 0.998f) * 500.0f : 0.0f;
                float star2 = (hash(dir.x*2.1f, dir.y*2.3f, dir.z*1.9f) > 0.995f) ?
                    (hash(dir.x*2.1f, dir.y*2.3f, dir.z*1.9f) - 0.995f) * 200.0f : 0.0f;
                float star3 = (hash(dir.x*3.7f, dir.y*3.1f, dir.z*4.3f) > 0.993f) ?
                    (hash(dir.x*3.7f, dir.y*3.1f, dir.z*4.3f) - 0.993f) * 140.0f : 0.0f;

                starBright = star1 + star2 * 0.6f + star3 * 0.3f;
                starBright = std::min(starBright, 1.5f);

                // Star color: mix of white, blue-white, yellow-white
                float starColorRNG = hash(dir.x * 7919.0f, dir.y * 5171.0f, dir.z * 3329.0f);
                float sr = 0.9f + starColorRNG * 0.1f;
                float sg = 0.85f + starColorRNG * 0.15f;
                float sb = 0.8f + (1.0f - starColorRNG) * 0.2f;

                r += starBright * sr;
                g += starBright * sg;
                b += starBright * sb;

                // Milky way band
                float galacticLat = std::abs(dir.y);
                // smoothstep: 3t^2 - 2t^3
                float t_band = std::min(std::max((galacticLat - 0.0f) / (0.35f - 0.0f), 0.0f), 1.0f);
                float bandFade = 1.0f - (t_band * t_band * (3.0f - 2.0f * t_band));
                float bandNoise = hash2D(dir.x * 5.0f, dir.z * 5.0f) * 0.5f + 0.5f;
                float band = bandFade * bandNoise * 0.06f;
                r += band;
                g += band * 0.9f;
                b += band * 1.1f;

                // Clamp
                int idx = (y * size + x) * 3;
                data[idx]     = (unsigned char)(std::min(r, 1.0f) * 255);
                data[idx + 1] = (unsigned char)(std::min(g, 1.0f) * 255);
                data[idx + 2] = (unsigned char)(std::min(b, 1.0f) * 255);
            }
        }
        return data;
    }
};
