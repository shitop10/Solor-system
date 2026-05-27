#pragma once
#include "MglMath.h"
#include <vector>
#include "Shader.h"

struct PointLight {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 ambient  = glm::vec3(0.05f);
    glm::vec3 diffuse  = glm::vec3(0.5f);
    glm::vec3 specular = glm::vec3(0.5f);
    float constant     = 1.0f;
    float linear       = 0.09f;
    float quadratic    = 0.032f;
};

struct DirLight {
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 ambient   = glm::vec3(0.1f);
    glm::vec3 diffuse   = glm::vec3(0.6f);
    glm::vec3 specular  = glm::vec3(0.4f);
};

class LightManager {
public:
    DirLight sun;
    std::vector<PointLight> pointLights;
    bool useDirLight = true;

    void apply(const Shader& shader) const {
        // Directional light
        shader.setBool("useDirLight", useDirLight);
        if (useDirLight) {
            shader.setVec3("dirLight.direction", sun.direction);
            shader.setVec3("dirLight.ambient",   sun.ambient);
            shader.setVec3("dirLight.diffuse",   sun.diffuse);
            shader.setVec3("dirLight.specular",  sun.specular);
        }

        // Point lights
        int count = (int)pointLights.size();
        shader.setInt("numPointLights", count);
        for (int i = 0; i < count; ++i) {
            std::string base = "pointLights[" + std::to_string(i) + "].";
            shader.setVec3(base + "position",  pointLights[i].position);
            shader.setVec3(base + "ambient",   pointLights[i].ambient);
            shader.setVec3(base + "diffuse",   pointLights[i].diffuse);
            shader.setVec3(base + "specular",  pointLights[i].specular);
            shader.setFloat(base + "constant",  pointLights[i].constant);
            shader.setFloat(base + "linear",    pointLights[i].linear);
            shader.setFloat(base + "quadratic", pointLights[i].quadratic);
        }
    }

    void updateSunPosition(const glm::vec3& sunWorldPos) {
        sun.direction = glm::normalize(-sunWorldPos);
    }
};
