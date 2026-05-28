#pragma once
#include "MglMath.h"
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>

enum CameraMode { FREE_CAMERA, ORBIT_CAMERA, TOP_DOWN_VIEW };

class Camera {
public:
    CameraMode mode = ORBIT_CAMERA;

    // ── Free camera ───────────────────────────────────
    glm::vec3 freePos    = glm::vec3(0.0f, 15.0f, 40.0f);
    float freeYaw        = -90.0f;
    float freePitch      = -15.0f;
    float freeSpeed      = 2.0f;
    float freeSensitivity = 0.15f;
    float freeSpeedBoost = 1.0f;    // multiplier (Shift = 2.5x)

    // ── Orbit camera ──────────────────────────────────
    glm::vec3 orbitTarget = glm::vec3(0.0f);
    std::string targetName = "Sun";
    float orbitRadius     = 30.0f;
    float orbitYaw        = -45.0f;
    float orbitPitch      = 30.0f;
    float orbitSensitivity = 0.15f;
    glm::vec3 panOffset   = glm::vec3(0.0f);  // independent pan from orbit center

    // ── Top-down view ────────────────────────────────
    float topDownHeight  = 80.0f;
    float topDownAngle   = 0.0f;     // rotation around Y axis
    glm::vec3 topDownCenter = glm::vec3(0.0f);

    // ── Smooth transition ─────────────────────────────
    glm::vec3 smoothTargetPos;
    glm::vec3 smoothCurrentPos;
    bool   transitioning = false;
    float  transitionProgress = 1.0f;
    float  transitionSpeed = 2.5f;   // seconds to complete

    // ── Projection ────────────────────────────────────
    float fov    = 60.0f;
    float nearP  = 0.1f;
    float farP   = 800.0f;

    // =================================================================
    //  View matrices
    // =================================================================
    glm::mat4 getViewMatrix() const {
        switch (mode) {
            case FREE_CAMERA:   return freeView();
            case ORBIT_CAMERA:  return orbitView();
            case TOP_DOWN_VIEW: return topDownView();
        }
        return glm::mat4(1.0f);
    }

    glm::mat4 getProjection(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, nearP, farP);
    }

    glm::vec3 getPosition() const {
        switch (mode) {
            case FREE_CAMERA:   return freePos;
            case ORBIT_CAMERA:  return orbitPos();
            case TOP_DOWN_VIEW: return topDownPos();
        }
        return glm::vec3(0.0f);
    }

    // =================================================================
    //  Input handlers
    // =================================================================
    void processMouseDrag(float dx, float dy) {
        if (transitioning) return;
        switch (mode) {
            case FREE_CAMERA:
                freeYaw   += dx * freeSensitivity;
                freePitch -= dy * freeSensitivity;
                freePitch  = glm::clamp(freePitch, -89.0f, 89.0f);
                break;
            case ORBIT_CAMERA:
                orbitYaw   += dx * orbitSensitivity;
                orbitPitch -= dy * orbitSensitivity;
                orbitPitch  = glm::clamp(orbitPitch, 2.0f, 88.0f);
                break;
            case TOP_DOWN_VIEW:
                topDownAngle += dx * 0.3f;
                topDownHeight -= dy * 2.0f;
                topDownHeight = glm::clamp(topDownHeight, 10.0f, 250.0f);
                break;
        }
    }

    void processScroll(float delta) {
        switch (mode) {
            case FREE_CAMERA:
                freePos += freeForward() * delta * 4.0f * freeSpeedBoost;
                break;
            case ORBIT_CAMERA:
                orbitRadius -= delta * 3.0f;
                orbitRadius  = glm::clamp(orbitRadius, 1.5f, 200.0f);
                break;
            case TOP_DOWN_VIEW:
                topDownHeight -= delta * 10.0f;
                topDownHeight = glm::clamp(topDownHeight, 10.0f, 250.0f);
                break;
        }
    }

    void processPan(float dx, float dy) {
        // Middle mouse drag: pan the view
        if (transitioning) return;
        float panSpeed = 0.05f;
        switch (mode) {
            case FREE_CAMERA:
                freePos += freeRight() * (-dx * panSpeed * freeSpeed);
                freePos += freeUp() * (dy * panSpeed * freeSpeed);
                break;
            case ORBIT_CAMERA:
                panOffset += freeRight() * (-dx * panSpeed * orbitRadius * 0.5f);
                panOffset += freeUp() * (dy * panSpeed * orbitRadius * 0.5f);
                break;
            case TOP_DOWN_VIEW: {
                float s = topDownHeight * 0.015f;
                float a = glm::radians(topDownAngle);
                topDownCenter.x += (-dx * cos(a) - dy * sin(a)) * s;
                topDownCenter.z += ( dx * sin(a) - dy * cos(a)) * s;
                break;
            }
        }
    }

    void setSpeedBoost(bool active) {
        freeSpeedBoost = active ? 2.5f : 1.0f;
    }

    void moveForward(float amount) {
        if (mode == FREE_CAMERA)
            freePos += freeForward() * amount * freeSpeed * freeSpeedBoost;
    }
    void moveRight(float amount) {
        if (mode == FREE_CAMERA)
            freePos += freeRight() * amount * freeSpeed * freeSpeedBoost;
    }
    void moveUp(float amount) {
        if (mode == FREE_CAMERA)
            freePos += glm::vec3(0.0f, 1.0f, 0.0f) * amount * freeSpeed * freeSpeedBoost;
    }

    // =================================================================
    //  Orbit target management
    // =================================================================
    void setOrbitTarget(const glm::vec3& pos, const std::string& name) {
        if (targetName == name && glm::length(orbitTarget - pos) < 0.01f) return;

        // Start smooth transition
        smoothCurrentPos = getPosition();
        smoothTargetPos = computeOrbitPosAround(pos, orbitRadius, orbitYaw, orbitPitch);
        orbitTarget = pos;
        targetName = name;
        panOffset = glm::vec3(0.0f);
        transitioning = true;
        transitionProgress = 0.0f;

        printf("[Camera] Orbit target: %s (%.1f, %.1f, %.1f)\n",
               name.c_str(), pos.x, pos.y, pos.z);
    }

    void resetView() {
        panOffset = glm::vec3(0.0f);
        switch (mode) {
            case FREE_CAMERA:
                freePos    = glm::vec3(0.0f, 15.0f, 40.0f);
                freeYaw    = -90.0f;
                freePitch  = -15.0f;
                break;
            case ORBIT_CAMERA:
                orbitRadius = 25.0f;
                orbitYaw    = -45.0f;
                orbitPitch  = 35.0f;
                break;
            case TOP_DOWN_VIEW:
                topDownHeight = 80.0f;
                topDownAngle  = 0.0f;
                topDownCenter = glm::vec3(0.0f);
                break;
        }
        printf("[Camera] View reset\n");
    }

    // ── Smooth transition update (call each frame) ─────
    void updateTransition(float dt) {
        if (!transitioning) return;
        transitionProgress += dt / transitionSpeed;
        if (transitionProgress >= 1.0f) {
            transitionProgress = 1.0f;
            transitioning = false;
        }
    }

    // ── Interpolated position for smooth transitions ───
    glm::vec3 getSmoothPosition() const {
        if (!transitioning) return getPosition();
        float t = smoothStep(transitionProgress);
        return smoothCurrentPos + (smoothTargetPos - smoothCurrentPos) * t;
    }

    const char* modeName() const {
        switch (mode) {
            case FREE_CAMERA:   return "Free";
            case ORBIT_CAMERA:  return "Orbit";
            case TOP_DOWN_VIEW: return "TopDown";
        }
        return "";
    }

private:
    // ── Free camera helpers ────────────────────────────
    glm::vec3 freeForward() const {
        float y = sin(glm::radians(freePitch));
        float x = cos(glm::radians(freePitch)) * cos(glm::radians(freeYaw));
        float z = cos(glm::radians(freePitch)) * sin(glm::radians(freeYaw));
        return glm::vec3(x, y, z);
    }
    glm::vec3 freeRight() const {
        return glm::normalize(glm::cross(freeForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    glm::vec3 freeUp() const {
        return glm::normalize(glm::cross(freeRight(), freeForward()));
    }

    // ── Orbit camera helpers ───────────────────────────
    glm::vec3 orbitPos() const {
        return computeOrbitPosAround(orbitTarget + panOffset, orbitRadius, orbitYaw, orbitPitch);
    }
    glm::vec3 computeOrbitPosAround(const glm::vec3& center, float radius, float yaw, float pitch) const {
        float y = radius * sin(glm::radians(pitch));
        float r = radius * cos(glm::radians(pitch));
        float x = r * cos(glm::radians(yaw));
        float z = r * sin(glm::radians(yaw));
        return center + glm::vec3(x, y, z);
    }

    // ── Top-down camera ────────────────────────────────
    glm::vec3 topDownPos() const {
        return topDownCenter + glm::vec3(0.0f, topDownHeight, 0.0f);
    }

    // ── View matrices ──────────────────────────────────
    glm::mat4 freeView() const {
        return glm::lookAt(freePos, freePos + freeForward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::mat4 orbitView() const {
        glm::vec3 eye = getSmoothPosition();
        glm::vec3 tgt = transitioning
            ? (orbitTarget + (smoothCurrentPos - smoothTargetPos) * 0.3f)
            : (orbitTarget + panOffset);
        return glm::lookAt(eye, tgt, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::mat4 topDownView() const {
        float a = glm::radians(topDownAngle);
        glm::vec3 offset(cos(a) * topDownHeight * 0.15f, 0.0f, sin(a) * topDownHeight * 0.15f);
        return glm::lookAt(topDownPos() + offset, topDownCenter,
                          glm::vec3(0.0f, 1.0f, 0.0f));
    }

    float smoothStep(float t) const {
        return t * t * (3.0f - 2.0f * t);
    }
};
