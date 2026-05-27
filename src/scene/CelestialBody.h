#pragma once
#include "MglMath.h"
#include <string>
#include <cmath>
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"

struct OrbitalElements {
    double semiMajorAxis;
    double eccentricity;
    double inclination;
    double longitudeAscendingNode;
    double argumentOfPeriapsis;
    double meanAnomaly;
    double period;

    OrbitalElements(double a = 0, double e = 0, double i = 0, double lan = 0,
                    double aop = 0, double ma = 0, double p = 1)
        : semiMajorAxis(a), eccentricity(e), inclination(i),
          longitudeAscendingNode(lan), argumentOfPeriapsis(aop),
          meanAnomaly(ma), period(p) {}
};

class CelestialBody {
public:
    std::string name;
    double mass;          // kg
    double radius;        // km
    double rotationPeriod; // Earth years

    OrbitalElements orbit;
    glm::dvec3 position = glm::dvec3(0.0);
    double orbitAngle   = 0.0;
    double rotationAngle = 0.0;

    // ── Rendering ─────────────────────────────────────
    Mesh sphereMesh;
    Mesh orbitLine;
    unsigned int diffuseTex  = 0;
    unsigned int specularTex = 0;
    unsigned int normalTex   = 0;
    Material material;
    bool hasTextures = false;
    float visualScale = 0.5f;

    static constexpr double AU_KM  = 149597870.7;
    static constexpr double G_AU   = 39.478;   // AU^3 / (Msun * yr^2)
    static constexpr double SOLAR_MASS = 1.9885e30;

    CelestialBody() = default;

    CelestialBody(const std::string& n, double m, double r, double rp, const OrbitalElements& oe)
        : name(n), mass(m), radius(r), rotationPeriod(rp), orbit(oe) {
        orbitAngle = glm::radians(orbit.meanAnomaly);
    }

    // ── Keplerian position update with eccentricity ──
    void updatePosition(double simYears) {
        if (orbit.period <= 0) return;  // Sun doesn't orbit

        double meanMotion = 2.0 * glm::pi<double>() / orbit.period;
        double meanAnomaly = glm::radians(orbit.meanAnomaly) + meanMotion * simYears;

        // Solve Kepler's equation: M = E - e*sin(E) via Newton's method
        double E = meanAnomaly;
        for (int iter = 0; iter < 10; iter++) {
            double dE = (E - orbit.eccentricity * sin(E) - meanAnomaly)
                      / (1.0 - orbit.eccentricity * cos(E));
            E -= dE;
            if (std::fabs(dE) < 1e-10) break;
        }

        double cosE = cos(E), sinE = sin(E);
        double semiMin = orbit.semiMajorAxis * sqrt(1.0 - orbit.eccentricity * orbit.eccentricity);

        // Position in orbital plane
        double xOrb = orbit.semiMajorAxis * (cosE - orbit.eccentricity);
        double yOrb = semiMin * sinE;

        // Rotate by argument of periapsis
        double aop = glm::radians(orbit.argumentOfPeriapsis);
        double cosAop = cos(aop), sinAop = sin(aop);
        double x1 = xOrb * cosAop - yOrb * sinAop;
        double y1 = xOrb * sinAop + yOrb * cosAop;

        // Rotate by inclination
        double inc = glm::radians(orbit.inclination);
        double cosInc = cos(inc), sinInc = sin(inc);
        double z1 = y1 * sinInc;
        double y2 = y1 * cosInc;

        // Rotate by longitude of ascending node
        double lan = glm::radians(orbit.longitudeAscendingNode);
        double cosLan = cos(lan), sinLan = sin(lan);
        position.x = x1 * cosLan - y2 * sinLan;
        position.y = z1;
        position.z = x1 * sinLan + y2 * cosLan;

        orbitAngle = atan2(position.z, position.x);
        rotationAngle += 2.0 * glm::pi<double>() * simYears / rotationPeriod;
    }

    // ── Visual radius for display ─────────────────────
    static constexpr float ORBIT_SCALE = 8.0f;

    float getVisualRadius() const {
        double earthRad = 6371.0;
        if (name == "Sun") return 5.0f;
        double ratio = radius / earthRad;
        // exponent 0.70 preserves relative size ordering better than 0.40
        // Jupiter ~4x Earth, Saturn ~3.5x, Uranus/Neptune ~2x
        return (float)(0.25 + 0.55 * pow(ratio, 0.70));
    }

    // ── World position (scaled for display) ────────────
    glm::vec3 worldPos() const {
        return glm::vec3(
            (float)(position.x * ORBIT_SCALE),
            (float)(position.y * ORBIT_SCALE),
            (float)(position.z * ORBIT_SCALE)
        );
    }

    // ── Render ────────────────────────────────────────
    void render(Shader& planetShader, Shader& ringShader, Shader& orbitShader,
                const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& viewPos, bool isSun) const
    {
        planetShader.use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, worldPos());
        model = glm::rotate(model, (float)rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        float r = getVisualRadius();
        model = glm::scale(model, glm::vec3(r));

        planetShader.setMat4("model", model);
        planetShader.setMat4("view", view);
        planetShader.setMat4("projection", projection);
        planetShader.setVec3("viewPos", viewPos);
        material.apply(planetShader);

        planetShader.setBool("useDiffuseMap", hasTextures && diffuseTex != 0);
        planetShader.setBool("useSpecularMap", hasTextures && specularTex != 0);
        planetShader.setBool("useNormalMap", hasTextures && normalTex != 0);

        if (hasTextures && diffuseTex != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTex);
            planetShader.setInt("diffuseMap", 0);
        }
        if (hasTextures && specularTex != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularTex);
            planetShader.setInt("specularMap", 1);
        }
        if (hasTextures && normalTex != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalTex);
            planetShader.setInt("normalMap", 2);
        }

        sphereMesh.bind();
        glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Saturn rings
        if (name == "Saturn") {
            ringShader.use();
            glm::mat4 ringModel = glm::mat4(1.0f);
            ringModel = glm::translate(ringModel, worldPos());
            ringModel = glm::scale(ringModel, glm::vec3(r * 1.4f));
            ringShader.setMat4("model", ringModel);
            ringShader.setMat4("view", view);
            ringShader.setMat4("projection", projection);
            ringShader.setVec4("ringColor", glm::vec4(0.8f, 0.75f, 0.6f, 0.7f));
            ringShader.setFloat("alpha", 0.7f);
            ringShader.setVec3("viewPos", viewPos);
            // ring mesh rendering handled by renderer
        }
    }

    void renderOrbit(Shader& orbitShader, const glm::mat4& view, const glm::mat4& projection) const {
        if (orbit.semiMajorAxis <= 0) return;
        orbitShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3((float)orbit.semiMajorAxis));
        orbitShader.setMat4("model", model);
        orbitShader.setMat4("view", view);
        orbitShader.setMat4("projection", projection);
        orbitLine.bind();
        glDrawArrays(GL_LINE_LOOP, 0, orbitLine.indexCount);
        glBindVertexArray(0);
    }
};
