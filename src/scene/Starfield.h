#pragma once
#include "MglMath.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include "Shader.h"
#include "Mesh.h"

struct StarVertex {
    glm::vec3 position;
    float brightness;
    float colorIdx;  // 0=cool white, 1=warm yellow, 2=blue-white
};

class Starfield {
public:
    Mesh starMesh;
    unsigned int starCount = 3000;

    void initialize() {
        std::srand(42);
        std::vector<StarVertex> stars(starCount);

        for (unsigned int i = 0; i < starCount; ++i) {
            // Uniform distribution on a sphere
            float theta = float(std::rand()) / RAND_MAX * 2.0f * glm::pi<float>();
            float phi   = std::acos(2.0f * float(std::rand()) / RAND_MAX - 1.0f);
            float r     = 380.0f + float(std::rand()) / RAND_MAX * 40.0f; // within far plane

            stars[i].position = glm::vec3(
                r * std::sin(phi) * std::cos(theta),
                r * std::sin(phi) * std::sin(theta),
                r * std::cos(phi)
            );

            // Brightness tiers: mostly dim, some medium, few bright
            float rnd = float(std::rand()) / RAND_MAX;
            if (rnd < 0.02f)
                stars[i].brightness = 1.8f + float(std::rand()) / RAND_MAX * 1.2f; // very bright
            else if (rnd < 0.10f)
                stars[i].brightness = 1.0f + float(std::rand()) / RAND_MAX * 0.8f;  // bright
            else if (rnd < 0.35f)
                stars[i].brightness = 0.5f + float(std::rand()) / RAND_MAX * 0.5f;  // medium
            else
                stars[i].brightness = 0.15f + float(std::rand()) / RAND_MAX * 0.35f; // dim

            // Color: mostly cool white, some warm, some blue
            float cr = float(std::rand()) / RAND_MAX;
            if (cr < 0.6f)
                stars[i].colorIdx = 0.0f; // cool white
            else if (cr < 0.85f)
                stars[i].colorIdx = 1.0f; // warm yellow
            else
                stars[i].colorIdx = 2.0f; // blue-white
        }

        // Upload to GPU
        glGenVertexArrays(1, &starMesh.VAO);
        glGenBuffers(1, &starMesh.VBO);
        glBindVertexArray(starMesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, starMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, stars.size() * sizeof(StarVertex),
                     stars.data(), GL_STATIC_DRAW);
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex),
                              (void*)offsetof(StarVertex, position));
        glEnableVertexAttribArray(0);
        // brightness
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(StarVertex),
                              (void*)offsetof(StarVertex, brightness));
        glEnableVertexAttribArray(1);
        // colorIdx
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(StarVertex),
                              (void*)offsetof(StarVertex, colorIdx));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

        starMesh.drawMode = GL_POINTS;
        starMesh.indexCount = starCount;
    }

    void render(Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                float time) const {
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setFloat("time", time);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glDepthMask(GL_FALSE);
        starMesh.bind();
        glDrawArrays(GL_POINTS, 0, starMesh.indexCount);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
    }
};
