#pragma once
#include "MglMath.h"
#include "Shader.h"

// ============================================================
//  Multi-Material System — PBR material definition
//  Supports: roughness/metallic workflow, anisotropy,
//            clearcoat, subsurface approximation, emission
// ============================================================

enum MaterialType {
    MAT_ROCKY = 0,       // Mercury, Moon, Asteroid
    MAT_GAS_GIANT,       // Jupiter, Saturn
    MAT_ICE_GIANT,       // Uranus, Neptune
    MAT_PLASMA,          // Sun
    MAT_CLOUD,           // Venus
    MAT_TERRESTRIAL,     // Earth
    MAT_LAVA,            // Io-like volcanic
    MAT_ICE_CRYSTAL,     // Enceladus-like ice
    MAT_METALLIC,        // metallic asteroid / spacecraft
    MAT_COUNT
};

struct Material {
    // ── Core PBR ────────────────────────────────────
    glm::vec3 ambient   = glm::vec3(0.15f);
    glm::vec3 diffuse   = glm::vec3(0.90f);
    glm::vec3 specular  = glm::vec3(0.5f);
    glm::vec3 emissive  = glm::vec3(0.0f);
    float shininess     = 0.5f;
    float roughness     = 0.5f;     // 0=smooth mirror, 1=matte
    float metallic      = 0.0f;     // 0=dielectric, 1=pure metal
    float alpha         = 1.0f;

    // ── Extended PBR ────────────────────────────────
    float anisotropy    = 0.0f;     // 0=isotropic, 1=fully anisotropic (for rings/hair)
    float subsurface    = 0.0f;     // SSS approximation weight
    float clearcoat     = 0.0f;     // clearcoat layer strength (ice, water)
    float clearcoatRoughness = 0.1f;

    // ── Metadata ────────────────────────────────────
    MaterialType type = MAT_ROCKY;
    const char* presetName = "Default";

    // ── Apply all uniforms to shader ────────────────
    void apply(const Shader& shader) const {
        shader.setVec3("material_ambient",   ambient);
        shader.setVec3("material_diffuse",   diffuse);
        shader.setVec3("material_specular",  specular);
        shader.setVec3("material_emissive",  emissive);
        shader.setFloat("material_shininess", shininess);
        shader.setFloat("material_roughness", roughness);
        shader.setFloat("material_metallic",  metallic);
        shader.setFloat("material_anisotropy", anisotropy);
        shader.setFloat("material_subsurface", subsurface);
        shader.setFloat("material_clearcoat", clearcoat);
        shader.setFloat("material_clearcoatRoughness", clearcoatRoughness);
    }

    // =================================================================
    //  Material Preset Factory — 13 distinct celestial materials
    // =================================================================

    // ── Sun: blazing plasma with dynamic granulation ─────
    static Material createSun() {
        Material m;
        m.type = MAT_PLASMA;
        m.presetName = "Solar Plasma";
        m.ambient   = glm::vec3(1.0f, 0.85f, 0.20f);
        m.diffuse   = glm::vec3(1.0f, 0.95f, 0.60f);
        m.specular  = glm::vec3(0.15f, 0.15f, 0.08f);
        m.emissive  = glm::vec3(2.5f, 1.4f, 0.18f);
        m.roughness = 0.18f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Mercury: heavily cratered grey rock, very rough ──
    static Material createMercury() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Cratered Regolith";
        m.ambient   = glm::vec3(0.08f, 0.08f, 0.09f);
        m.diffuse   = glm::vec3(0.82f, 0.82f, 0.85f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.05f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.92f;
        m.metallic  = 0.04f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Venus: thick sulfuric cloud layer, soft diffuse ──
    static Material createVenus() {
        Material m;
        m.type = MAT_CLOUD;
        m.presetName = "Sulfuric Clouds";
        m.ambient   = glm::vec3(0.18f, 0.14f, 0.07f);
        m.diffuse   = glm::vec3(0.90f, 0.83f, 0.58f);
        m.specular  = glm::vec3(0.12f, 0.10f, 0.06f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.72f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.15f;   // thick atmosphere scatters light
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Earth: oceans + continents, moderate specular ────
    static Material createEarth() {
        Material m;
        m.type = MAT_TERRESTRIAL;
        m.presetName = "Oceanic Terrestrial";
        m.ambient   = glm::vec3(0.06f, 0.08f, 0.15f);
        m.diffuse   = glm::vec3(0.95f, 0.95f, 0.95f);
        m.specular  = glm::vec3(0.55f, 0.55f, 0.58f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.38f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.08f;    // thin atmosphere
        m.clearcoat  = 0.30f;    // ocean specular (boosted for visibility)
        m.clearcoatRoughness = 0.10f;
        return m;
    }

    // ── Mars: iron oxide rust, rough porous surface ──────
    static Material createMars() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Iron Oxide Desert";
        m.ambient   = glm::vec3(0.10f, 0.04f, 0.02f);
        m.diffuse   = glm::vec3(0.88f, 0.52f, 0.28f);
        m.specular  = glm::vec3(0.06f, 0.03f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.85f;
        m.metallic  = 0.08f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Jupiter: banded gas giant, turbulent atmosphere ──
    static Material createJupiter() {
        Material m;
        m.type = MAT_GAS_GIANT;
        m.presetName = "Banded Gas Giant";
        m.ambient   = glm::vec3(0.12f, 0.10f, 0.06f);
        m.diffuse   = glm::vec3(0.90f, 0.78f, 0.52f);
        m.specular  = glm::vec3(0.04f, 0.03f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.80f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.08f;    // deep atmosphere scattering
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Saturn: pale yellow, smooth banded gas ───────────
    static Material createSaturn() {
        Material m;
        m.type = MAT_GAS_GIANT;
        m.presetName = "Pale Gas Giant";
        m.ambient   = glm::vec3(0.15f, 0.12f, 0.07f);
        m.diffuse   = glm::vec3(0.93f, 0.88f, 0.68f);
        m.specular  = glm::vec3(0.05f, 0.04f, 0.03f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.76f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.06f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Uranus: smooth ice giant, cyan with high specular ─
    static Material createUranus() {
        Material m;
        m.type = MAT_ICE_GIANT;
        m.presetName = "Smooth Ice Giant";
        m.ambient   = glm::vec3(0.08f, 0.13f, 0.16f);
        m.diffuse   = glm::vec3(0.78f, 0.88f, 0.94f);
        m.specular  = glm::vec3(0.48f, 0.50f, 0.55f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.22f;
        m.metallic  = 0.03f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.25f;    // boosted: icy subsurface scattering
        m.clearcoat  = 0.45f;    // boosted: icy surface glaze
        m.clearcoatRoughness = 0.08f;
        return m;
    }

    // ── Neptune: deep blue ice giant, moderate specular ──
    static Material createNeptune() {
        Material m;
        m.type = MAT_ICE_GIANT;
        m.presetName = "Deep Ice Giant";
        m.ambient   = glm::vec3(0.05f, 0.08f, 0.16f);
        m.diffuse   = glm::vec3(0.68f, 0.76f, 0.94f);
        m.specular  = glm::vec3(0.42f, 0.45f, 0.52f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.25f;
        m.metallic  = 0.03f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.22f;    // boosted
        m.clearcoat  = 0.40f;    // boosted
        m.clearcoatRoughness = 0.08f;
        return m;
    }

    // ── Moon: heavily cratered grey rock, matte ──────────
    static Material createMoon() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Lunar Regolith";
        m.ambient   = glm::vec3(0.06f, 0.06f, 0.07f);
        m.diffuse   = glm::vec3(0.75f, 0.75f, 0.78f);
        m.specular  = glm::vec3(0.03f, 0.03f, 0.04f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.94f;
        m.metallic  = 0.02f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Asteroid: dark carbonaceous rock ────────────────
    static Material createAsteroid() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Carbonaceous Rock";
        m.ambient   = glm::vec3(0.04f, 0.04f, 0.03f);
        m.diffuse   = glm::vec3(0.52f, 0.45f, 0.38f);
        m.specular  = glm::vec3(0.02f, 0.02f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.96f;
        m.metallic  = 0.06f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── NEW: Lava — volcanic, self-luminous, dynamic ─────
    static Material createLava() {
        Material m;
        m.type = MAT_LAVA;
        m.presetName = "Volcanic Lava";
        m.ambient   = glm::vec3(0.30f, 0.06f, 0.02f);
        m.diffuse   = glm::vec3(0.85f, 0.25f, 0.04f);
        m.specular  = glm::vec3(0.10f, 0.05f, 0.02f);
        m.emissive  = glm::vec3(1.80f, 0.40f, 0.03f);
        m.roughness = 0.50f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.30f;    // lava glow spreads beneath surface
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── NEW: Ice Crystal — high transparency, specular ────
    static Material createIceCrystal() {
        Material m;
        m.type = MAT_ICE_CRYSTAL;
        m.presetName = "Crystalline Ice";
        m.ambient   = glm::vec3(0.12f, 0.15f, 0.20f);
        m.diffuse   = glm::vec3(0.70f, 0.82f, 0.95f);
        m.specular  = glm::vec3(0.90f, 0.92f, 0.95f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.08f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.15f;    // ice has slight anisotropy from crystal structure
        m.subsurface = 0.40f;    // strong subsurface scattering in ice
        m.clearcoat  = 0.55f;    // glossy ice surface
        m.clearcoatRoughness = 0.05f;
        return m;
    }

    // ── NEW: Metallic Asteroid — high metal content ──────
    static Material createMetallicAsteroid() {
        Material m;
        m.type = MAT_METALLIC;
        m.presetName = "Metallic Asteroid";
        m.ambient   = glm::vec3(0.08f, 0.07f, 0.06f);
        m.diffuse   = glm::vec3(0.55f, 0.50f, 0.45f);
        m.specular  = glm::vec3(0.65f, 0.60f, 0.55f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.35f;
        m.metallic  = 0.85f;     // highly metallic
        m.anisotropy = 0.30f;    // brushed metal look
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── NEW: Martian Desert — finer variant of Mars ──────
    static Material createDesertPlanet() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Sandy Desert";
        m.ambient   = glm::vec3(0.15f, 0.10f, 0.05f);
        m.diffuse   = glm::vec3(0.92f, 0.72f, 0.42f);
        m.specular  = glm::vec3(0.06f, 0.05f, 0.03f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.88f;
        m.metallic  = 0.02f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.02f;
        m.clearcoat  = 0.0f;
        return m;
    }
};
