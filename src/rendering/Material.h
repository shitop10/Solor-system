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

    // ── Sun: photosphere ~5778 K, warm-white self-luminous plasma ──
    static Material createSun() {
        Material m;
        m.type = MAT_PLASMA;
        m.presetName = "Solar Photosphere";
        m.ambient   = glm::vec3(1.0f, 0.92f, 0.65f);
        m.diffuse   = glm::vec3(1.0f, 0.97f, 0.82f);
        m.specular  = glm::vec3(0.06f, 0.06f, 0.05f);
        m.emissive  = glm::vec3(2.8f, 2.0f, 0.9f);
        m.roughness = 0.12f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Mercury: ~0.12 albedo, cratered regolith, no atmosphere ──
    static Material createMercury() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Cratered Regolith";
        m.ambient   = glm::vec3(0.05f, 0.05f, 0.06f);
        m.diffuse   = glm::vec3(0.62f, 0.62f, 0.65f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.04f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.88f;
        m.metallic  = 0.03f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Venus: ~0.75 albedo, thick H₂SO₄ cloud deck, diffuse ──
    static Material createVenus() {
        Material m;
        m.type = MAT_CLOUD;
        m.presetName = "Sulfuric Cloud Deck";
        m.ambient   = glm::vec3(0.12f, 0.10f, 0.06f);
        m.diffuse   = glm::vec3(0.88f, 0.82f, 0.58f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.03f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.60f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.28f;
        m.clearcoat  = 0.06f;
        m.clearcoatRoughness = 0.40f;
        return m;
    }

    // ── Earth: ~0.30 albedo, ocean Fresnel + Rayleigh atmosphere ──
    static Material createEarth() {
        Material m;
        m.type = MAT_TERRESTRIAL;
        m.presetName = "Oceanic Terrestrial";
        m.ambient   = glm::vec3(0.05f, 0.07f, 0.14f);
        m.diffuse   = glm::vec3(0.95f, 0.95f, 0.95f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.04f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.42f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.08f;
        m.clearcoat  = 0.28f;
        m.clearcoatRoughness = 0.12f;
        return m;
    }

    // ── Mars: ~0.25 albedo, iron-oxide regolith, dusty desert ──
    static Material createMars() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Iron Oxide Desert";
        m.ambient   = glm::vec3(0.08f, 0.04f, 0.02f);
        m.diffuse   = glm::vec3(0.78f, 0.42f, 0.20f);
        m.specular  = glm::vec3(0.04f, 0.03f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.82f;
        m.metallic  = 0.05f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.02f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Jupiter: ~0.52 albedo, turbulent banded atmosphere ──
    static Material createJupiter() {
        Material m;
        m.type = MAT_GAS_GIANT;
        m.presetName = "Banded Gas Giant";
        m.ambient   = glm::vec3(0.10f, 0.08f, 0.05f);
        m.diffuse   = glm::vec3(0.88f, 0.76f, 0.50f);
        m.specular  = glm::vec3(0.03f, 0.03f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.72f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.06f;
        m.subsurface = 0.12f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Saturn: ~0.47 albedo, pale banded gas, subtler than Jupiter ──
    static Material createSaturn() {
        Material m;
        m.type = MAT_GAS_GIANT;
        m.presetName = "Pale Gas Giant";
        m.ambient   = glm::vec3(0.12f, 0.10f, 0.06f);
        m.diffuse   = glm::vec3(0.90f, 0.85f, 0.65f);
        m.specular  = glm::vec3(0.03f, 0.03f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.68f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.04f;
        m.subsurface = 0.10f;
        m.clearcoat  = 0.03f;
        m.clearcoatRoughness = 0.50f;
        return m;
    }

    // ── Uranus: ~0.51 albedo, smooth featureless ice giant, retrograde ──
    static Material createUranus() {
        Material m;
        m.type = MAT_ICE_GIANT;
        m.presetName = "Smooth Ice Giant";
        m.ambient   = glm::vec3(0.06f, 0.12f, 0.15f);
        m.diffuse   = glm::vec3(0.75f, 0.85f, 0.92f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.04f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.18f;
        m.metallic  = 0.02f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.28f;
        m.clearcoat  = 0.42f;
        m.clearcoatRoughness = 0.06f;
        return m;
    }

    // ── Neptune: ~0.41 albedo, deep blue ice giant, subtle storms ──
    static Material createNeptune() {
        Material m;
        m.type = MAT_ICE_GIANT;
        m.presetName = "Deep Ice Giant";
        m.ambient   = glm::vec3(0.04f, 0.07f, 0.15f);
        m.diffuse   = glm::vec3(0.62f, 0.72f, 0.92f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.04f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.22f;
        m.metallic  = 0.02f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.25f;
        m.clearcoat  = 0.38f;
        m.clearcoatRoughness = 0.07f;
        return m;
    }

    // ── Moon: ~0.12 albedo, anorthosite regolith, heavily cratered ──
    static Material createMoon() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Lunar Regolith";
        m.ambient   = glm::vec3(0.04f, 0.04f, 0.05f);
        m.diffuse   = glm::vec3(0.60f, 0.60f, 0.63f);
        m.specular  = glm::vec3(0.03f, 0.03f, 0.03f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.92f;
        m.metallic  = 0.02f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Asteroid: C-type carbonaceous, ~0.03–0.09 albedo, very dark ──
    static Material createAsteroid() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Carbonaceous Chondrite";
        m.ambient   = glm::vec3(0.03f, 0.03f, 0.02f);
        m.diffuse   = glm::vec3(0.42f, 0.38f, 0.32f);
        m.specular  = glm::vec3(0.02f, 0.02f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.94f;
        m.metallic  = 0.04f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Lava: Io-like volcanic, ~1200–1500 K thermal emission ──
    static Material createLava() {
        Material m;
        m.type = MAT_LAVA;
        m.presetName = "Silicate Melt";
        m.ambient   = glm::vec3(0.30f, 0.06f, 0.02f);
        m.diffuse   = glm::vec3(0.82f, 0.24f, 0.04f);
        m.specular  = glm::vec3(0.08f, 0.06f, 0.04f);
        m.emissive  = glm::vec3(2.2f, 0.50f, 0.06f);
        m.roughness = 0.44f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.38f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Ice Crystal: Enceladus-like, ~0.99 albedo, fresh water ice ──
    static Material createIceCrystal() {
        Material m;
        m.type = MAT_ICE_CRYSTAL;
        m.presetName = "Crystalline Water Ice";
        m.ambient   = glm::vec3(0.10f, 0.13f, 0.18f);
        m.diffuse   = glm::vec3(0.68f, 0.80f, 0.94f);
        m.specular  = glm::vec3(0.04f, 0.04f, 0.04f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.08f;
        m.metallic  = 0.0f;
        m.anisotropy = 0.15f;
        m.subsurface = 0.45f;
        m.clearcoat  = 0.52f;
        m.clearcoatRoughness = 0.05f;
        return m;
    }

    // ── Metallic Asteroid: M-type, nickel-iron, radar-bright ──
    static Material createMetallicAsteroid() {
        Material m;
        m.type = MAT_METALLIC;
        m.presetName = "Nickel-Iron Meteorite";
        m.ambient   = glm::vec3(0.06f, 0.05f, 0.04f);
        m.diffuse   = glm::vec3(0.48f, 0.43f, 0.38f);
        m.specular  = glm::vec3(0.58f, 0.53f, 0.46f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.30f;
        m.metallic  = 0.90f;
        m.anisotropy = 0.28f;
        m.subsurface = 0.0f;
        m.clearcoat  = 0.0f;
        return m;
    }

    // ── Desert Planet: warm sandy arid world, thin atmosphere ──
    static Material createDesertPlanet() {
        Material m;
        m.type = MAT_ROCKY;
        m.presetName = "Arid Desert";
        m.ambient   = glm::vec3(0.12f, 0.08f, 0.04f);
        m.diffuse   = glm::vec3(0.86f, 0.66f, 0.36f);
        m.specular  = glm::vec3(0.04f, 0.03f, 0.02f);
        m.emissive  = glm::vec3(0.0f);
        m.roughness = 0.85f;
        m.metallic  = 0.02f;
        m.anisotropy = 0.0f;
        m.subsurface = 0.03f;
        m.clearcoat  = 0.0f;
        return m;
    }
};
