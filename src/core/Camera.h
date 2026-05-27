#pragma once
#include "MglMath.h"
#include <cmath>

enum CameraMode { FREE_CAMERA, ORBIT_CAMERA, SYSTEM_VIEW };

class Camera {
public:
    CameraMode mode = ORBIT_CAMERA;

    // ── Free camera ───────────────────────────────────
    glm::vec3 freePos    = glm::vec3(0.0f, 8.0f, 25.0f);
    float freeYaw        = -90.0f;
    float freePitch      = -20.0f;
    float freeSpeed      = 8.0f;
    float freeSensitivity = 0.15f;

    // ── Orbit camera ──────────────────────────────────
    glm::vec3 orbitTarget = glm::vec3(0.0f);
    float orbitRadius     = 60.0f;
    float orbitYaw        = -45.0f;
    float orbitPitch      = 30.0f;
    float orbitSensitivity = 0.15f;

    // ── System view ───────────────────────────────────
    float systemZoom = 50.0f;

    // ── Projection ────────────────────────────────────
    float fov    = 60.0f;
    float nearP  = 0.1f;
    float farP   = 500.0f;

    glm::mat4 getViewMatrix() const {
        switch (mode) {
            case FREE_CAMERA: return freeView();
            case ORBIT_CAMERA: return orbitView();
            case SYSTEM_VIEW: return systemView();
        }
        return glm::mat4(1.0f);
    }

    glm::mat4 getProjection(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, nearP, farP);
    }

    glm::vec3 getPosition() const {
        switch (mode) {
            case FREE_CAMERA: return freePos;
            case ORBIT_CAMERA: return orbitPos();
            case SYSTEM_VIEW: return systemPos();
        }
        return glm::vec3(0.0f);
    }

    // ── Input handlers ────────────────────────────────
    void processMouseDrag(float dx, float dy) {
        switch (mode) {
            case FREE_CAMERA:
                freeYaw   += dx * freeSensitivity;
                freePitch -= dy * freeSensitivity;
                freePitch  = glm::clamp(freePitch, -89.0f, 89.0f);
                break;
            case ORBIT_CAMERA:
                orbitYaw   += dx * orbitSensitivity;
                orbitPitch -= dy * orbitSensitivity;
                orbitPitch  = glm::clamp(orbitPitch, 1.0f, 89.0f);
                break;
            default: break;
        }
    }

    void processScroll(float delta) {
        switch (mode) {
            case FREE_CAMERA:
                freePos += freeForward() * delta * 2.0f;
                break;
            case ORBIT_CAMERA:
                orbitRadius -= delta;
                orbitRadius  = glm::clamp(orbitRadius, 2.0f, 100.0f);
                break;
            case SYSTEM_VIEW:
                systemZoom -= delta * 5.0f;
                systemZoom  = glm::clamp(systemZoom, 10.0f, 200.0f);
                break;
        }
    }

    void moveForward(float amount) {
        if (mode == FREE_CAMERA)
            freePos += freeForward() * amount * freeSpeed;
    }
    void moveRight(float amount) {
        if (mode == FREE_CAMERA)
            freePos += freeRight() * amount * freeSpeed;
    }
    void moveUp(float amount) {
        if (mode == FREE_CAMERA)
            freePos += glm::vec3(0.0f, 1.0f, 0.0f) * amount * freeSpeed;
    }

    void updateFollowTarget(const glm::vec3& target) {
        orbitTarget = target;
    }

    const char* modeName() const {
        switch (mode) {
            case FREE_CAMERA:  return "Free Camera";
            case ORBIT_CAMERA: return "Orbit Camera";
            case SYSTEM_VIEW:  return "System View";
        }
        return "";
    }

private:
    glm::vec3 freeForward() const {
        float y = sin(glm::radians(freePitch));
        float x = cos(glm::radians(freePitch)) * cos(glm::radians(freeYaw));
        float z = cos(glm::radians(freePitch)) * sin(glm::radians(freeYaw));
        return glm::vec3(x, y, z);
    }
    glm::vec3 freeRight() const {
        return glm::normalize(glm::cross(freeForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    glm::vec3 orbitPos() const {
        float y = orbitRadius * sin(glm::radians(orbitPitch));
        float r = orbitRadius * cos(glm::radians(orbitPitch));
        float x = r * cos(glm::radians(orbitYaw));
        float z = r * sin(glm::radians(orbitYaw));
        return orbitTarget + glm::vec3(x, y, z);
    }
    glm::vec3 systemPos() const {
        return glm::vec3(0.0f, systemZoom * 0.5f, systemZoom);
    }

    glm::mat4 freeView() const {
        return glm::lookAt(freePos, freePos + freeForward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::mat4 orbitView() const {
        return glm::lookAt(orbitPos(), orbitTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::mat4 systemView() const {
        return glm::lookAt(systemPos(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
};
