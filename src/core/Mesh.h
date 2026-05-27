#pragma once
#include <glad/glad.h>
#include "MglMath.h"
#include <vector>
#include <cmath>
#include <cstddef>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 texCoord;
};

struct Mesh {
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int indexCount = 0;
    GLenum drawMode = GL_TRIANGLES;

    void bind() const { glBindVertexArray(VAO); }
    void draw() const {
        glBindVertexArray(VAO);
        if (indexCount > 0)
            glDrawElements(drawMode, indexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(drawMode, 0, 0);
    }
    void cleanup() {
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
    }

    // ── Factory: UV Sphere ────────────────────────────
    static Mesh createSphere(float radius, int sectors, int stacks) {
        Mesh m;
        std::vector<Vertex> verts;
        std::vector<unsigned int> indices;

        for (int y = 0; y <= stacks; ++y) {
            float phi = glm::pi<float>() * float(y) / float(stacks);
            for (int x = 0; x <= sectors; ++x) {
                float theta = 2.0f * glm::pi<float>() * float(x) / float(sectors);
                Vertex v;
                v.position = glm::vec3(
                    radius * sin(phi) * cos(theta),
                    radius * cos(phi),
                    radius * sin(phi) * sin(theta)
                );
                v.normal   = glm::normalize(v.position);

                // Tangent: direction of increasing u (theta, around equator)
                // ∂p/∂θ = (-r*sin(φ)*sin(θ), 0, r*sin(φ)*cos(θ))
                v.tangent = glm::normalize(glm::vec3(
                    -sin(theta), 0.0f, cos(theta)
                ));

                // Bitangent: direction of increasing v (phi, pole to pole)
                // ∂p/∂φ = (r*cos(φ)*cos(θ), -r*sin(φ), r*cos(φ)*sin(θ))
                v.bitangent = glm::normalize(glm::vec3(
                    cos(phi) * cos(theta), -sin(phi), cos(phi) * sin(theta)
                ));

                v.texCoord = glm::vec2(float(x) / float(sectors), float(y) / float(stacks));
                verts.push_back(v);
            }
        }

        for (int y = 0; y < stacks; ++y) {
            for (int x = 0; x < sectors; ++x) {
                unsigned int a = y * (sectors + 1) + x;
                unsigned int b = a + sectors + 1;
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
                indices.push_back(a + 1);
                indices.push_back(b);
                indices.push_back(b + 1);
            }
        }

        m.indexCount = (unsigned int)indices.size();
        uploadMesh(m, verts, indices);
        return m;
    }

    // ── Factory: Ring (Saturn) ────────────────────────
    static Mesh createRing(float innerR, float outerR, int segments) {
        Mesh m;
        std::vector<Vertex> verts;
        std::vector<unsigned int> indices;

        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * float(i) / float(segments);
            float c = cos(angle), s = sin(angle);

            // Ring normal points up (Y+), tangent follows the ring,
            // bitangent is radial
            glm::vec3 ringNormal   = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 ringTangent  = glm::normalize(glm::vec3(-s, 0.0f, c));
            glm::vec3 ringBitangent = glm::normalize(glm::vec3(c, 0.0f, s));

            Vertex vi, vo;
            vi.position  = glm::vec3(innerR * c, 0.0f, innerR * s);
            vi.normal    = ringNormal;
            vi.tangent   = ringTangent;
            vi.bitangent = ringBitangent;
            vi.texCoord  = glm::vec2(0.0f, float(i) / float(segments));
            vo.position  = glm::vec3(outerR * c, 0.0f, outerR * s);
            vo.normal    = ringNormal;
            vo.tangent   = ringTangent;
            vo.bitangent = ringBitangent;
            vo.texCoord  = glm::vec2(1.0f, float(i) / float(segments));

            verts.push_back(vi);
            verts.push_back(vo);
        }

        for (int i = 0; i < segments; ++i) {
            unsigned int a = i * 2, b = i * 2 + 1;
            unsigned int c = (i + 1) * 2, d = (i + 1) * 2 + 1;
            indices.push_back(a); indices.push_back(c); indices.push_back(b);
            indices.push_back(b); indices.push_back(c); indices.push_back(d);
        }

        m.indexCount = (unsigned int)indices.size();
        uploadMesh(m, verts, indices);
        return m;
    }

    // ── Factory: Quad (HUD / fullscreen) ──────────────
    // Note: Uses raw float array to avoid dependency on Vertex struct layout
    static Mesh createQuad() {
        Mesh m;
        // pos(3) + normal(3) + tangent(3) + bitangent(3) + uv(2) = 14 floats
        float q[] = {
            // pos              normal       tangent       bitangent      uv
            -1, 1, 0,  0,0,1,  1,0,0, 0,1,0,  0,1,
            -1,-1, 0,  0,0,1,  1,0,0, 0,1,0,  0,0,
             1,-1, 0,  0,0,1,  1,0,0, 0,1,0,  1,0,
             1, 1, 0,  0,0,1,  1,0,0, 0,1,0,  1,1,
        };
        unsigned int idx[] = { 0,1,2, 0,2,3 };

        glGenVertexArrays(1, &m.VAO);
        glGenBuffers(1, &m.VBO);
        glGenBuffers(1, &m.EBO);
        glBindVertexArray(m.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        const int stride = 14 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)(12 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glBindVertexArray(0);
        m.indexCount = 6;
        return m;
    }

    // ── Raw points (stars) ────────────────────────────
    static Mesh createPoints(const std::vector<glm::vec3>& positions) {
        Mesh m;
        m.drawMode = GL_POINTS;
        glGenVertexArrays(1, &m.VAO);
        glGenBuffers(1, &m.VBO);
        glBindVertexArray(m.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
                     positions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        m.indexCount = (unsigned int)positions.size();
        return m;
    }

    // ── Orbit line ────────────────────────────────────
    static Mesh createOrbitLine(float radius, int segments) {
        Mesh m;
        m.drawMode = GL_LINE_LOOP;
        std::vector<glm::vec3> pts;
        for (int i = 0; i < segments; ++i) {
            float a = 2.0f * glm::pi<float>() * float(i) / float(segments);
            pts.push_back(glm::vec3(radius * cos(a), 0.0f, radius * sin(a)));
        }
        glGenVertexArrays(1, &m.VAO);
        glGenBuffers(1, &m.VBO);
        glBindVertexArray(m.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
        glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec3), pts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        m.indexCount = (unsigned int)pts.size();
        return m;
    }

private:
    static void uploadMesh(Mesh& m, const std::vector<Vertex>& verts,
                           const std::vector<unsigned int>& indices) {
        glGenVertexArrays(1, &m.VAO);
        glGenBuffers(1, &m.VBO);
        glGenBuffers(1, &m.EBO);
        glBindVertexArray(m.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                     indices.data(), GL_STATIC_DRAW);
        // location 0: position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        // location 1: normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        // location 2: texCoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);
        // location 3: tangent
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(3);
        // location 4: bitangent
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
        glEnableVertexAttribArray(4);
        glBindVertexArray(0);
    }
};
