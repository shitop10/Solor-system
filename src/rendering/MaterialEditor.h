#pragma once
#include "Material.h"
#include "CelestialBody.h"
#include <vector>
#include <memory>
#include <string>
#include <cstdio>

// ============================================================
//  Runtime Material Editor
//  Select a body and tweak its PBR parameters in real time
// ============================================================
class MaterialEditor {
public:
    bool active = false;
    int selectedBodyIndex = -1;
    CelestialBody* targetBody = nullptr;
    std::vector<std::shared_ptr<CelestialBody>>* bodyList = nullptr;

    // Which parameter is being edited
    enum Param {
        PARAM_ROUGHNESS = 0,
        PARAM_METALLIC,
        PARAM_ANISOTROPY,
        PARAM_SUBSURFACE,
        PARAM_CLEARCOAT,
        PARAM_CLEARCOAT_ROUGH,
        PARAM_EMISSIVE_R,
        PARAM_EMISSIVE_G,
        PARAM_EMISSIVE_B,
        PARAM_COUNT
    };
    int currentParam = PARAM_ROUGHNESS;

    static const char* paramName(Param p) {
        switch (p) {
            case PARAM_ROUGHNESS:       return "Roughness";
            case PARAM_METALLIC:        return "Metallic";
            case PARAM_ANISOTROPY:      return "Anisotropy";
            case PARAM_SUBSURFACE:      return "Subsurface";
            case PARAM_CLEARCOAT:       return "Clearcoat";
            case PARAM_CLEARCOAT_ROUGH: return "Clearcoat Rough";
            case PARAM_EMISSIVE_R:      return "Emissive R";
            case PARAM_EMISSIVE_G:      return "Emissive G";
            case PARAM_EMISSIVE_B:      return "Emissive B";
            default: return "?";
        }
    }

    void setBodyList(std::vector<std::shared_ptr<CelestialBody>>* list) {
        bodyList = list;
    }

    // ── Select next body ────────────────────────────────
    void nextBody() {
        if (!bodyList || bodyList->empty()) return;
        selectedBodyIndex = (selectedBodyIndex + 1) % (int)bodyList->size();
        targetBody = (*bodyList)[selectedBodyIndex].get();
        printf("[MatEdit] Selected: %s (preset: %s)\n",
               targetBody->name.c_str(), targetBody->material.presetName);
    }

    void prevBody() {
        if (!bodyList || bodyList->empty()) return;
        selectedBodyIndex = (selectedBodyIndex - 1 + (int)bodyList->size()) % (int)bodyList->size();
        targetBody = (*bodyList)[selectedBodyIndex].get();
        printf("[MatEdit] Selected: %s (preset: %s)\n",
               targetBody->name.c_str(), targetBody->material.presetName);
    }

    // ── Cycle parameter ─────────────────────────────────
    void nextParam() {
        currentParam = (currentParam + 1) % PARAM_COUNT;
        if (targetBody) printCurrent();
    }

    void prevParam() {
        currentParam = (currentParam - 1 + PARAM_COUNT) % PARAM_COUNT;
        if (targetBody) printCurrent();
    }

    // ── Adjust current parameter ────────────────────────
    void adjust(float delta) {
        if (!targetBody) return;
        Material& m = targetBody->material;

        switch ((Param)currentParam) {
            case PARAM_ROUGHNESS:
                m.roughness = clampVal(m.roughness + delta * 0.02f, 0.01f, 1.0f);
                break;
            case PARAM_METALLIC:
                m.metallic = clampVal(m.metallic + delta * 0.02f, 0.0f, 1.0f);
                break;
            case PARAM_ANISOTROPY:
                m.anisotropy = clampVal(m.anisotropy + delta * 0.02f, 0.0f, 1.0f);
                break;
            case PARAM_SUBSURFACE:
                m.subsurface = clampVal(m.subsurface + delta * 0.02f, 0.0f, 1.0f);
                break;
            case PARAM_CLEARCOAT:
                m.clearcoat = clampVal(m.clearcoat + delta * 0.02f, 0.0f, 1.0f);
                break;
            case PARAM_CLEARCOAT_ROUGH:
                m.clearcoatRoughness = clampVal(m.clearcoatRoughness + delta * 0.02f, 0.01f, 1.0f);
                break;
            case PARAM_EMISSIVE_R:
                m.emissive.x = clampVal(m.emissive.x + delta * 0.05f, 0.0f, 5.0f);
                break;
            case PARAM_EMISSIVE_G:
                m.emissive.y = clampVal(m.emissive.y + delta * 0.05f, 0.0f, 5.0f);
                break;
            case PARAM_EMISSIVE_B:
                m.emissive.z = clampVal(m.emissive.z + delta * 0.05f, 0.0f, 5.0f);
                break;
            default: break;
        }
    }

    // ── Reset material to preset ────────────────────────
    void resetToPreset() {
        if (!targetBody) return;
        // Remap planet name to preset
        if (targetBody->name == "Sun")      targetBody->material = Material::createSun();
        if (targetBody->name == "Mercury")  targetBody->material = Material::createMercury();
        if (targetBody->name == "Venus")    targetBody->material = Material::createVenus();
        if (targetBody->name == "Earth")    targetBody->material = Material::createEarth();
        if (targetBody->name == "Mars")     targetBody->material = Material::createMars();
        if (targetBody->name == "Jupiter")  targetBody->material = Material::createJupiter();
        if (targetBody->name == "Saturn")   targetBody->material = Material::createSaturn();
        if (targetBody->name == "Uranus")   targetBody->material = Material::createUranus();
        if (targetBody->name == "Neptune")  targetBody->material = Material::createNeptune();
        if (targetBody->name == "Moon")     targetBody->material = Material::createMoon();
        printf("[MatEdit] %s material reset to default\n", targetBody->name.c_str());
    }

    // ── Apply lava preset ───────────────────────────────
    void applyLavaPreset() {
        if (!targetBody) return;
        targetBody->material = Material::createLava();
        printf("[MatEdit] %s <- Lava preset\n", targetBody->name.c_str());
    }

    // ── Apply ice preset ────────────────────────────────
    void applyIcePreset() {
        if (!targetBody) return;
        targetBody->material = Material::createIceCrystal();
        printf("[MatEdit] %s <- Ice Crystal preset\n", targetBody->name.c_str());
    }

    // ── Apply metallic asteroid preset ──────────────────
    void applyMetalPreset() {
        if (!targetBody) return;
        targetBody->material = Material::createMetallicAsteroid();
        printf("[MatEdit] %s <- Metallic Asteroid preset\n", targetBody->name.c_str());
    }

    // ── Format current state as string ──────────────────
    std::string getStatusString() const {
        if (!targetBody || !active) return "";
        Material& m = targetBody->material;
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Edit: %s | %s | R=%.2f M=%.2f A=%.2f SS=%.2f CC=%.2f CR=%.2f E=(%.1f,%.1f,%.1f)",
            targetBody->name.c_str(),
            paramName((Param)currentParam),
            m.roughness, m.metallic, m.anisotropy,
            m.subsurface, m.clearcoat, m.clearcoatRoughness,
            m.emissive.x, m.emissive.y, m.emissive.z);
        return std::string(buf);
    }

private:
    float clampVal(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    void printCurrent() {
        if (!targetBody) return;
        Material& m = targetBody->material;
        float val = 0.0f;
        switch ((Param)currentParam) {
            case PARAM_ROUGHNESS:       val = m.roughness; break;
            case PARAM_METALLIC:        val = m.metallic; break;
            case PARAM_ANISOTROPY:      val = m.anisotropy; break;
            case PARAM_SUBSURFACE:      val = m.subsurface; break;
            case PARAM_CLEARCOAT:       val = m.clearcoat; break;
            case PARAM_CLEARCOAT_ROUGH: val = m.clearcoatRoughness; break;
            case PARAM_EMISSIVE_R:      val = m.emissive.x; break;
            case PARAM_EMISSIVE_G:      val = m.emissive.y; break;
            case PARAM_EMISSIVE_B:      val = m.emissive.z; break;
            default: break;
        }
        printf("[MatEdit] %s.%s = %.3f\n", targetBody->name.c_str(), paramName((Param)currentParam), val);
    }
};
