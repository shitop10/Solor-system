#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <cmath>
#include "CelestialBody.h"
#include "Mesh.h"
#include "Material.h"
#include "TextureGenerator.h"

// Simple asteroid particle for the belt
struct AsteroidParticle {
    glm::vec3 position;
    float scale;
    float angle;
    float orbitSpeed;
    float orbitRadius;
    float yOffset;
};

class SolarSystem {
public:
    std::vector<std::shared_ptr<CelestialBody>> bodies;
    double simulationTime = 0.0;
    double timeScale = 10.0;
    bool paused = false;

    Mesh sphereTemplate;
    Mesh ringTemplate;
    Mesh uranusRingMesh;       // thin ring for Uranus
    std::map<std::string, Mesh> orbitTemplates;

    // Asteroid belt
    std::vector<AsteroidParticle> asteroids;
    Mesh asteroidMesh;         // small rock mesh for instances
    bool showAsteroids = true;
    unsigned int asteroidTex = 0;

    // Moon
    std::shared_ptr<CelestialBody> moon;

    void initialize() {
        sphereTemplate  = Mesh::createSphere(1.0f, 48, 24);
        ringTemplate    = Mesh::createRing(1.0f, 1.8f, 80);
        uranusRingMesh  = Mesh::createRing(1.35f, 1.65f, 80);  // thinner, further out

        // ── Sun ───────────────────────────────────────────
        auto sun = std::make_shared<CelestialBody>(
            "Sun", 1.9885e30, 696342.0, 25.05,
            OrbitalElements(0, 0, 0, 0, 0, 0, 0));
        sun->material = Material::createSun();
        sun->diffuseTex = TextureGenerator::generateSunTexture(1024);
        sun->hasTextures = true;
        bodies.push_back(sun);

        // ── Mercury ───────────────────────────────────────
        auto mercury = std::make_shared<CelestialBody>(
            "Mercury", 3.3011e23, 2439.7, 58.646,
            OrbitalElements(0.3871, 0.2056, 7.005, 48.331, 29.124, 174.796, 0.2408));
        mercury->material = Material::createMercury();
        mercury->diffuseTex = TextureGenerator::generateRockyPlanetTexture(512, glm::vec3(0.50f, 0.50f, 0.53f));
        mercury->normalTex  = TextureGenerator::generateNormalMap(512, 5.0f);
        mercury->hasTextures = true;
        bodies.push_back(mercury);

        // ── Venus ─────────────────────────────────────────
        auto venus = std::make_shared<CelestialBody>(
            "Venus", 4.8675e24, 6051.8, -243.025,
            OrbitalElements(0.7233, 0.0068, 3.3946, 76.680, 54.884, 50.115, 0.6152));
        venus->material = Material::createVenus();
        venus->diffuseTex = TextureGenerator::generateGasGiantTexture(512,
            glm::vec3(0.82f, 0.72f, 0.40f), glm::vec3(0.95f, 0.88f, 0.60f));
        venus->normalTex  = TextureGenerator::generateNormalMap(512, 2.0f);
        venus->hasTextures = true;
        bodies.push_back(venus);

        // ── Earth ─────────────────────────────────────────
        auto earth = std::make_shared<CelestialBody>(
            "Earth", 5.97237e24, 6371.0, 1.0,
            OrbitalElements(1.0, 0.0167, 0.0, 0.0, 102.947, 100.464, 1.0));
        earth->material = Material::createEarth();
        earth->diffuseTex = TextureGenerator::generateEarthTexture(1024);
        earth->normalTex = TextureGenerator::generateNormalMap(512, 3.0f);
        earth->specularTex = TextureGenerator::generateSpecularMap(512, 0.35f);
        earth->hasTextures = true;
        bodies.push_back(earth);

        // ── Mars ──────────────────────────────────────────
        auto mars = std::make_shared<CelestialBody>(
            "Mars", 6.4171e23, 3389.5, 1.025,
            OrbitalElements(1.5237, 0.0934, 1.850, 49.558, 286.502, 19.413, 1.8808));
        mars->material = Material::createMars();
        mars->diffuseTex = TextureGenerator::generateRockyPlanetTexture(512, glm::vec3(0.72f, 0.25f, 0.12f));
        mars->normalTex  = TextureGenerator::generateNormalMap(512, 6.0f);
        mars->hasTextures = true;
        bodies.push_back(mars);

        // ── Asteroid belt ─────────────────────────────────
        initAsteroidBelt();

        // ── Jupiter ───────────────────────────────────────
        auto jupiter = std::make_shared<CelestialBody>(
            "Jupiter", 1.8986e27, 69911.0, 0.4135,
            OrbitalElements(5.2029, 0.0489, 1.303, 100.556, 273.867, 20.020, 11.862));
        jupiter->material = Material::createJupiter();
        jupiter->diffuseTex = TextureGenerator::generateJupiterTexture(1024);
        jupiter->normalTex  = TextureGenerator::generateNormalMap(512, 1.5f);
        jupiter->hasTextures = true;
        bodies.push_back(jupiter);

        // ── Saturn ────────────────────────────────────────
        auto saturn = std::make_shared<CelestialBody>(
            "Saturn", 5.6834e26, 58232.0, 0.444,
            OrbitalElements(9.537, 0.0542, 2.485, 113.665, 336.014, 317.020, 29.457));
        saturn->material = Material::createSaturn();
        saturn->diffuseTex = TextureGenerator::generateSaturnTexture(1024);
        saturn->normalTex  = TextureGenerator::generateNormalMap(512, 1.5f);
        saturn->hasTextures = true;
        bodies.push_back(saturn);

        // ── Uranus ────────────────────────────────────────
        auto uranus = std::make_shared<CelestialBody>(
            "Uranus", 8.6813e25, 25362.0, -0.718,
            OrbitalElements(19.189, 0.0473, 0.772, 74.016, 170.964, 142.239, 84.020));
        uranus->material = Material::createUranus();
        uranus->diffuseTex = TextureGenerator::generateGasGiantTexture(1024,
            glm::vec3(0.35f, 0.65f, 0.78f), glm::vec3(0.50f, 0.80f, 0.90f));
        uranus->normalTex  = TextureGenerator::generateNormalMap(512, 1.8f);
        uranus->specularTex = TextureGenerator::generateSpecularMap(512, 0.4f);
        uranus->hasTextures = true;
        bodies.push_back(uranus);

        // ── Neptune ───────────────────────────────────────
        auto neptune = std::make_shared<CelestialBody>(
            "Neptune", 1.0243e26, 24622.0, 0.671,
            OrbitalElements(30.070, 0.0086, 1.768, 131.784, 265.647, 256.228, 164.791));
        neptune->material = Material::createNeptune();
        neptune->diffuseTex = TextureGenerator::generateGasGiantTexture(1024,
            glm::vec3(0.12f, 0.28f, 0.65f), glm::vec3(0.28f, 0.48f, 0.85f));
        neptune->normalTex  = TextureGenerator::generateNormalMap(512, 2.0f);
        neptune->specularTex = TextureGenerator::generateSpecularMap(512, 0.35f);
        neptune->hasTextures = true;
        bodies.push_back(neptune);

        // ── Moon (orbiting Earth) ─────────────────────────
        moon = std::make_shared<CelestialBody>(
            "Moon", 7.342e22, 1737.4, 27.322,
            OrbitalElements(0.00257, 0.0549, 5.145, 0, 0, 0, 0.0748));
        moon->material = Material::createMoon();
        moon->diffuseTex = TextureGenerator::generateRockyPlanetTexture(256, glm::vec3(0.52f, 0.52f, 0.54f));
        moon->normalTex  = TextureGenerator::generateNormalMap(256, 6.0f);
        moon->hasTextures = true;
        moon->sphereMesh = sphereTemplate;
        // Moon visual radius is small
        moon->visualScale = 0.06f;

        // ── Assign shared meshes and orbit lines ──────────
        for (auto& b : bodies) {
            b->sphereMesh = sphereTemplate;
            float orbitR = (float)CelestialBody::toDisplayRadius(b->orbit.semiMajorAxis);
            if (b->orbit.semiMajorAxis > 0)
                b->orbitLine = Mesh::createOrbitLine(orbitR, 200);
        }
    }

    // ── Initialize asteroid belt ──────────────────────────
    void initAsteroidBelt() {
        const int count = 3000;
        asteroids.resize(count);

        // Pre-allocated position buffer for batched rendering
        std::vector<glm::vec3> positions(count);

        float innerAU = 2.1f;
        float outerAU = 3.3f;

        for (int i = 0; i < count; i++) {
            float rnd = (float)std::rand() / RAND_MAX;
            float orbitAU = innerAU + (outerAU - innerAU) * (rnd * 0.7f + 0.15f);

            asteroids[i].orbitRadius = (float)CelestialBody::toDisplayRadius(orbitAU);
            asteroids[i].angle = (float)std::rand() / RAND_MAX * 6.28318f;
            asteroids[i].orbitSpeed = 1.0f / (orbitAU * std::sqrt(orbitAU)) * 0.5f;
            asteroids[i].yOffset = ((float)std::rand() / RAND_MAX - 0.5f) * 1.5f;
            asteroids[i].scale = 1.0f;

            // Initial position
            float x = asteroids[i].orbitRadius * cos(asteroids[i].angle);
            float z = asteroids[i].orbitRadius * sin(asteroids[i].angle);
            positions[i] = glm::vec3(x, asteroids[i].yOffset, z);
        }

        // Create batched position buffer
        glGenVertexArrays(1, &asteroidMesh.VAO);
        glGenBuffers(1, &asteroidMesh.VBO);
        glBindVertexArray(asteroidMesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, asteroidMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
                     positions.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        asteroidMesh.indexCount = (unsigned int)positions.size();
        asteroidMesh.drawMode = GL_POINTS;

        asteroidTex = TextureGenerator::generateRockyPlanetTexture(64, glm::vec3(0.45f, 0.38f, 0.32f));
        printf("[SolarSystem] Asteroid belt: %d particles, batched VBO\n", count);
    }

    // ── Update asteroid belt positions (called each frame) ─
    void updateAsteroidBuffer() {
        if (!showAsteroids || asteroids.empty()) return;
        std::vector<glm::vec3> positions(asteroids.size());
        for (size_t i = 0; i < asteroids.size(); i++) {
            float x = asteroids[i].orbitRadius * cos(asteroids[i].angle);
            float z = asteroids[i].orbitRadius * sin(asteroids[i].angle);
            positions[i] = glm::vec3(x, asteroids[i].yOffset, z);
        }
        glBindBuffer(GL_ARRAY_BUFFER, asteroidMesh.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());
    }

    // ── Update positions ──────────────────────────────────
    void update(double deltaRealSeconds) {
        if (paused) return;
        double simYears = deltaRealSeconds * timeScale / 365.25;
        simulationTime += simYears;

        for (auto& b : bodies)
            b->updatePosition(simulationTime);

        // Update asteroid belt positions
        for (auto& a : asteroids)
            a.angle += (float)(simYears * a.orbitSpeed);

        // Moon: orbit around Earth — only update orbital angle,
        // position is computed relative to Earth in main rendering
        if (moon) {
            moon->updatePosition(simulationTime);
            moon->rotationAngle += 2.0 * glm::pi<double>() * simYears / moon->rotationPeriod;
        }
    }

    glm::vec3 sunPosition() const {
        if (!bodies.empty()) return bodies[0]->worldPos();
        return glm::vec3(0.0f);
    }

    // ── Get Earth position for Moon rendering ─────────────
    glm::vec3 earthWorldPos() const {
        for (auto& b : bodies)
            if (b->name == "Earth") return b->worldPos();
        return glm::vec3(0.0f);
    }
};
