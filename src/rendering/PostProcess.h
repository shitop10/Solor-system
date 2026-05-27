#pragma once
#include <glad/glad.h>
#include <cstdio>

// ======================================================
//  Post-process pipeline: Bloom (FBO + two-pass blur)
//
//  Pipeline:  scene FBO → bright extract → blur H → blur V → composite
// ======================================================
class PostProcess {
public:
    bool enabled = true;

    // FBOs for off-screen rendering
    unsigned int sceneFBO = 0, sceneTex = 0, sceneRBO = 0;
    unsigned int brightFBO = 0, brightTex = 0;
    unsigned int blurFBO[2] = {0, 0},  // ping-pong
                  blurTex[2] = {0, 0};

    // Fullscreen quad
    unsigned int quadVAO = 0, quadVBO = 0;

    int width = 1280, height = 720;

    // Bloom parameters
    float threshold = 0.8f;
    float intensity = 0.6f;

    // ── Initialize FBOs and quad ──────────────────────
    void init(int w, int h) {
        width = w; height = h;
        cleanup();

        // Fullscreen quad
        float quad[] = {
            -1,1, 0,1,  -1,-1, 0,0,  1,-1, 1,0,
            -1,1, 0,1,  1,-1, 1,0,   1,1,  1,1
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        // Scene FBO (color + depth)
        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        glGenTextures(1, &sceneTex);
        glBindTexture(GL_TEXTURE_2D, sceneTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

        glGenRenderbuffers(1, &sceneRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("[PostProcess] ERROR: Scene FBO incomplete! Disabling bloom.\n");
            enabled = false;
        }

        // Ping-pong blur textures (half resolution for performance)
        int hw = w / 2, hh = h / 2;
        for (int i = 0; i < 2; i++) {
            glGenFramebuffers(1, &blurFBO[i]);
            glGenTextures(1, &blurTex[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);
            glBindTexture(GL_TEXTURE_2D, blurTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, hw, hh, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex[i], 0);
        }

        // Bright pass FBO
        glGenFramebuffers(1, &brightFBO);
        glGenTextures(1, &brightTex);
        glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
        glBindTexture(GL_TEXTURE_2D, brightTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, hw, hh, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightTex, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        printf("[PostProcess] Bloom pipeline initialized (%dx%d, half-res %dx%d)\n", w, h, hw, hh);
    }

    // ── Begin scene rendering to FBO ──────────────────
    void beginScene() {
        if (!enabled) return;
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // ── End scene FBO, run bloom, composite to screen ──
    void endScene(unsigned int extractShader, unsigned int blurShader,
                  unsigned int compositeShader) {
        if (!enabled) {
            // Copy scene FBO to default framebuffer if it was used
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }

        int hw = width / 2, hh = height / 2;

        // 1. Bright pass: downsample to half-res, keep only bright pixels
        glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
        glViewport(0, 0, hw, hh);
        glUseProgram(extractShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTex);
        glUniform1i(glGetUniformLocation(extractShader, "sceneTex"), 0);
        glUniform1f(glGetUniformLocation(extractShader, "threshold"), threshold);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. Horizontal blur: brightTex → blurTex[0]
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
        glViewport(0, 0, hw, hh);
        glUseProgram(blurShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, brightTex);
        glUniform1i(glGetUniformLocation(blurShader, "image"), 0);
        glUniform1f(glGetUniformLocation(blurShader, "texelSize"), 1.0f / hw);
        glUniform1f(glGetUniformLocation(blurShader, "dirX"), 1.0f);
        glUniform1f(glGetUniformLocation(blurShader, "dirY"), 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 3. Vertical blur: blurTex[0] → blurTex[1]
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[1]);
        glBindTexture(GL_TEXTURE_2D, blurTex[0]);
        glUniform1i(glGetUniformLocation(blurShader, "image"), 0);
        glUniform1f(glGetUniformLocation(blurShader, "texelSize"), 1.0f / hh);
        glUniform1f(glGetUniformLocation(blurShader, "dirX"), 0.0f);
        glUniform1f(glGetUniformLocation(blurShader, "dirY"), 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 4. Composite: sceneTex + blurTex[1] → default FB
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glUseProgram(compositeShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTex);
        glUniform1i(glGetUniformLocation(compositeShader, "sceneTex"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blurTex[1]);
        glUniform1i(glGetUniformLocation(compositeShader, "bloomTex"), 1);
        glUniform1f(glGetUniformLocation(compositeShader, "bloomIntensity"), intensity);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void toggle() { enabled = !enabled; printf("[PostProcess] Bloom: %s\n", enabled ? "ON" : "OFF"); }

    void cleanup() {
        auto del = [](unsigned int& id) { if (id) glDeleteTextures(1, &id); id = 0; };
        del(sceneTex); del(brightTex); del(blurTex[0]); del(blurTex[1]);
        auto delFB = [](unsigned int& id) { if (id) glDeleteFramebuffers(1, &id); id = 0; };
        delFB(sceneFBO); delFB(brightFBO); delFB(blurFBO[0]); delFB(blurFBO[1]);
        auto delRB = [](unsigned int& id) { if (id) glDeleteRenderbuffers(1, &id); id = 0; };
        delRB(sceneRBO);
        if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
        if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    }
};
