#pragma once
#include <glad/glad.h>
#include <vector>
#include <cstring>
#include <cstdio>

// ======================================================
//  Software Z-Buffer — educational CPU-side depth buffer
//
//  Demonstrates how a Z-Buffer works by reading back the
//  GPU depth buffer and visualizing it as a grayscale
//  overlay. The depth values (0=near, 1=far) are mapped
//  to brightness (near=bright, far=dark).
// ======================================================
class ZBuffer {
public:
    int width = 0, height = 0;
    std::vector<float> depthData;       // CPU-side depth values [0,1]
    std::vector<unsigned char> vizData; // RGBA visualization

    unsigned int vizTexID = 0;          // OpenGL texture for depth viz
    unsigned int vizVAO = 0, vizVBO = 0;
    bool showOverlay = false;
    float overlayAlpha = 0.7f;
    float overlayX = 0.0f, overlayY = 0.0f;  // bottom-right corner (NDC)
    float overlayW = 0.35f, overlayH = 0.30f;

    // ── Initialize with screen dimensions ────────────────
    void init(int w, int h) {
        width = w; height = h;
        size_t size = (size_t)w * h;
        depthData.resize(size);
        vizData.resize(size * 4);

        // Create visualization texture
        glGenTextures(1, &vizTexID);
        glBindTexture(GL_TEXTURE_2D, vizTexID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Full-screen quad VAO for viz overlay
        float quad[] = {
            // pos (NDC)          uv
            -1.0f,  1.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
            -1.0f,  1.0f,   0.0f, 1.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f,
        };

        glGenVertexArrays(1, &vizVAO);
        glGenBuffers(1, &vizVBO);
        glBindVertexArray(vizVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vizVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        printf("[ZBuffer] Software Z-Buffer initialized: %dx%d (%.1f KB)\n",
               w, h, (size * sizeof(float)) / 1024.0);
    }

    // ── Read back GPU depth buffer to CPU ────────────────
    void captureFromGPU() {
        if (width <= 0 || height <= 0) return;
        glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT,
                     GL_FLOAT, depthData.data());
    }

    // ── Generate depth visualization ─────────────────────
    //    Maps depth [0,1] to grayscale: near→bright, far→dark
    //    Also applies a color ramp for better readability
    void generateVisualization() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                float d = depthData[idx];

                // Non-linear remap: emphasize near range
                float dVis = 1.0f - d;  // near=bright, far=dark
                dVis = dVis * dVis * 0.7f + dVis * 0.3f; // gamma-like

                // Color ramp: blue(near) → cyan → green → yellow → red(far)
                unsigned char r, g, b;
                if (dVis < 0.25f) {
                    // Deep: dark blue
                    float t = dVis / 0.25f;
                    r = (unsigned char)(10 * t);
                    g = (unsigned char)(20 * t);
                    b = (unsigned char)(60 + 150 * t);
                } else if (dVis < 0.50f) {
                    // Mid: cyan to green
                    float t = (dVis - 0.25f) / 0.25f;
                    r = (unsigned char)(10 + 80 * t);
                    g = (unsigned char)(20 + 180 * t);
                    b = (unsigned char)(210 - 160 * t);
                } else if (dVis < 0.75f) {
                    // Near-mid: green to yellow
                    float t = (dVis - 0.50f) / 0.25f;
                    r = (unsigned char)(90 + 165 * t);
                    g = (unsigned char)(200 - 60 * t);
                    b = (unsigned char)(50 - 40 * t);
                } else {
                    // Very near: yellow to white
                    float t = (dVis - 0.75f) / 0.25f;
                    r = (unsigned char)(255);
                    g = (unsigned char)(140 + 115 * t);
                    b = (unsigned char)(10 + 150 * t);
                }

                int vidx = idx * 4;
                vizData[vidx + 0] = r;
                vizData[vidx + 1] = g;
                vizData[vidx + 2] = b;
                vizData[vidx + 3] = 255;
            }
        }
    }

    // ── Upload visualization to GPU texture ──────────────
    void uploadVisualization() {
        glBindTexture(GL_TEXTURE_2D, vizTexID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        GL_RGBA, GL_UNSIGNED_BYTE, vizData.data());
    }

    // ── Full update cycle: capture → visualize → upload ─
    void update() {
        if (!showOverlay) return;
        captureFromGPU();
        generateVisualization();
        uploadVisualization();
    }

    // ── Render depth overlay using inline shader ─────────
    void renderOverlay(unsigned int shaderID) {
        if (!showOverlay) return;
        glUseProgram(shaderID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vizTexID);
        glUniform1i(glGetUniformLocation(shaderID, "depthTex"), 0);
        glUniform1f(glGetUniformLocation(shaderID, "alpha"), overlayAlpha);
        glBindVertexArray(vizVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    // ── Print depth statistics to console ────────────────
    void printStats() const {
        if (depthData.empty()) return;
        float minD = 1.0f, maxD = 0.0f, sum = 0.0f;
        int validPixels = 0;
        for (float d : depthData) {
            if (d < 1.0f) { // skip far-plane (no geometry)
                if (d < minD) minD = d;
                if (d > maxD) maxD = d;
                sum += d;
                validPixels++;
            }
        }
        if (validPixels > 0) {
            printf("[ZBuffer] %d valid pixels | min=%.4f max=%.4f avg=%.4f\n",
                   validPixels, minD, maxD, sum / validPixels);
        }
    }

    void cleanup() {
        if (vizTexID) glDeleteTextures(1, &vizTexID);
        if (vizVAO)   glDeleteVertexArrays(1, &vizVAO);
        if (vizVBO)   glDeleteBuffers(1, &vizVBO);
    }
};
