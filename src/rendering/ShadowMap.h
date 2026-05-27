#pragma once
#include <glad/glad.h>
#include "MglMath.h"
#include <cstdio>

// ============================================================
//  Shadow Map — directional light depth map with PCF support
//  Uses a 2048x2048 depth texture for the main light (Sun)
// ============================================================
class ShadowMap {
public:
    unsigned int depthFBO = 0;
    unsigned int depthTex = 0;
    int resolution = 2048;

    // Light-space transform matrix
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

    // Config
    float nearPlane = 0.5f;
    float farPlane  = 500.0f;
    float orthoSize = 120.0f;  // covers full solar system (Neptune ~240 units out)

    bool enabled = true;

    void init(int res = 2048) {
        resolution = res;
        cleanup();

        // Depth texture
        glGenTextures(1, &depthTex);
        glBindTexture(GL_TEXTURE_2D, depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // Depth FBO
        glGenFramebuffers(1, &depthFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("[ShadowMap] ERROR: FBO incomplete!\n");
            enabled = false;
        } else {
            printf("[ShadowMap] Initialized %dx%d depth map (ortho=%.0f)\n", resolution, resolution, orthoSize);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ── Update light-space matrix ─────────────────────
    void updateLightMatrix(const glm::vec3& sunPos) {
        // Solar system: sun at origin, planets orbit in XZ plane.
        // Place the directional light camera high above, looking down at angle,
        // so shadows cast outward and slightly downward across the orbital plane.
        glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);

        // Light direction: from upper-right, casting shadows to lower-left
        // This gives visible shadow elongation on planets
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.35f, -1.0f, 0.25f));

        // Position light camera far from scene center along light direction
        float lightDist = 150.0f;
        glm::vec3 lightPos = lightTarget - lightDir * lightDist;

        // Orthographic projection
        glm::mat4 lightProjection = glm::ortho(
            -orthoSize, orthoSize,
            -orthoSize, orthoSize,
            nearPlane, farPlane);

        // Look-at: light camera looks at scene center
        glm::mat4 lightView = glm::lookAt(lightPos, lightTarget,
            glm::vec3(0.0f, 1.0f, 0.0f));

        lightSpaceMatrix = lightProjection * lightView;
    }

    // ── Begin shadow pass — render depth from light ──────
    void beginPass() {
        if (!enabled) return;
        glViewport(0, 0, resolution, resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);  // Peter Panning mitigation
    }

    // ── End shadow pass ──────────────────────────────────
    void endPass(int screenW, int screenH) {
        if (!enabled) return;
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenW, screenH);
    }

    // ── Bind depth texture for shader sampling ───────────
    void bindDepthTex(int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, depthTex);
    }

    void toggle() {
        enabled = !enabled;
        printf("[ShadowMap] Shadows: %s\n", enabled ? "ON" : "OFF");
    }

    void cleanup() {
        if (depthFBO) { glDeleteFramebuffers(1, &depthFBO); depthFBO = 0; }
        if (depthTex) { glDeleteTextures(1, &depthTex); depthTex = 0; }
    }
};
