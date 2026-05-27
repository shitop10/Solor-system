#include <glad/glad.h>
#include "MglMath.h"
#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Material.h"
#include "Light.h"
#include "CelestialBody.h"
#include "SolarSystem.h"
#include "Starfield.h"
#include "ZBuffer.h"
#include "PostProcess.h"
#include "ShadowMap.h"
#include "EnvironmentMap.h"
#include "MaterialEditor.h"
#include "FPSCounter.h"
#include <cstdio>
#include <cmath>
#include <windows.h>

// ── Global state ──────────────────────────────────────
SolarSystem   solarSystem;
Camera        camera;
Starfield     starfield;
ZBuffer       zBuffer;
PostProcess   postProcess;
ShadowMap     shadowMap;
EnvironmentMap envMap;
MaterialEditor matEditor;
FPSCounter    fpsCounter;
LightManager  lights;
Window        window;

Shader* planetShader   = nullptr;
Shader* ringShader     = nullptr;
Shader* starShader     = nullptr;
Shader* orbitShader    = nullptr;
Shader* glowShader     = nullptr;
Shader* zPrepassShader = nullptr;
Shader* shadowShader   = nullptr;
unsigned int hudShaderID = 0;
unsigned int depthVizShaderID = 0;
unsigned int bloomExtractID = 0, bloomBlurID = 0, bloomCompositeID = 0;

bool showOrbits   = true;
bool fogEnabled   = true;
bool showDepthViz = false;
bool zPrepassEnabled = true;
bool manualCulling = false;
bool bloomEnabled = false;
bool shadowEnabled = true;
bool scrollSunMode = false;   // L key: scroll adjusts sun intensity instead of zoom
float sunIntensity = 1.5f;    // sun brightness multiplier
int  fogModeIdx = 2;

// ── Per-key edge-detection state ──
struct KeyState {
    bool k1,k2,k3, sp, eq, min, o, f, z, b, v, g, a, h;
    bool m, n, p, up, dn, lt, rt, k5, k6, k7, k8;
    bool r, tab, mmb, l;
} prevKey = {};

// ── Inline orbit shader ───────────────────────────────
const char* orbitVertSrc = R"(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model, view, projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* orbitFragSrc = R"(#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() { FragColor = vec4(uColor, 0.5); }
)";

// ── Inline depth-visualization shader ───────────────────
const char* depthVizVertSrc = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* depthVizFragSrc = R"(#version 330 core
in vec2 vTexCoord;
uniform sampler2D depthTex;
uniform float alpha;
out vec4 FragColor;
void main() {
    vec4 col = texture(depthTex, vTexCoord);
    FragColor = vec4(col.rgb, alpha);
}
)";

// ── Post-process vertex shader (shared) ──────────────────
const char* postVertSrc = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// ── Bloom: bright-pass extract ────────────────────────────
const char* bloomExtractFragSrc = R"(#version 330 core
in vec2 vTexCoord;
uniform sampler2D sceneTex;
uniform float threshold;
out vec4 FragColor;
void main() {
    vec3 color = texture(sceneTex, vTexCoord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    FragColor = brightness > threshold ? vec4(color, 1.0) : vec4(0.0);
}
)";

// ── Bloom: Gaussian blur (dual-direction) ────────────────
const char* bloomBlurFragSrc = R"(#version 330 core
in vec2 vTexCoord;
uniform sampler2D image;
uniform float texelSize;
uniform float dirX, dirY;
out vec4 FragColor;
const float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
void main() {
    vec2 step = vec2(dirX, dirY) * texelSize;
    vec3 result = texture(image, vTexCoord).rgb * w[0];
    for (int i=1; i<5; i++) {
        result += texture(image, vTexCoord+step*float(i)).rgb * w[i];
        result += texture(image, vTexCoord-step*float(i)).rgb * w[i];
    }
    FragColor = vec4(result, 1.0);
}
)";

// ── Bloom: composite + tone mapping ──────────────────────
const char* bloomCompositeFragSrc = R"(#version 330 core
in vec2 vTexCoord;
uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomIntensity;
out vec4 FragColor;
void main() {
    vec3 scene = texture(sceneTex, vTexCoord).rgb;
    vec3 bloom = texture(bloomTex, vTexCoord).rgb;
    scene += bloom * bloomIntensity;
    scene = scene / (scene + vec3(1.0));          // Reinhard tonemap
    scene = pow(scene, vec3(1.0 / 2.2));          // gamma
    FragColor = vec4(scene, 1.0);
}
)";

unsigned int compileInlineShader(const char* vertSrc, const char* fragSrc) {
    auto compile = [](unsigned int type, const char* src) {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char info[512]; glGetShaderInfoLog(s, 512, nullptr, info);
                   printf("Shader compile error: %s\n", info); }
        return s;
    };
    unsigned int v = compile(GL_VERTEX_SHADER, vertSrc);
    unsigned int f = compile(GL_FRAGMENT_SHADER, fragSrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, v); glAttachShader(prog, f);
    glLinkProgram(prog);
    int ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char info[512]; glGetProgramInfoLog(prog, 512, nullptr, info);
               printf("Link error: %s\n", info); }
    glDeleteShader(v); glDeleteShader(f);
    return prog;
}

// ── Simple edge-detect helper ──────────────────────────
static bool edge(bool cur, bool& prev) {
    if (cur && !prev) { prev = cur; return true; }
    prev = cur;
    return false;
}

// ── Input processing ──────────────────────────────────
void processInput() {
    // Helper: check key + edge detect
    bool k[256];
    for (int i = 0; i < 256; i++) k[i] = window.keys[i];

    // Camera modes
    if (edge(k['1'], prevKey.k1)) { camera.mode = FREE_CAMERA; printf("[Camera] Free mode\n"); }
    if (edge(k['2'], prevKey.k2)) { camera.mode = ORBIT_CAMERA; printf("[Camera] Orbit mode\n"); }
    if (edge(k['3'], prevKey.k3)) { camera.mode = TOP_DOWN_VIEW; printf("[Camera] Top-down mode\n"); }

    // Cycle orbit target (Tab key)
    if (edge(k[VK_TAB], prevKey.tab)) {
        int curIdx = -1;
        for (int i = 0; i < (int)solarSystem.bodies.size(); i++) {
            if (solarSystem.bodies[i]->name == camera.targetName) { curIdx = i; break; }
        }
        int nextIdx = (curIdx + 1) % (int)solarSystem.bodies.size();
        auto& body = solarSystem.bodies[nextIdx];
        camera.setOrbitTarget(body->worldPos(), body->name);
    }

    // Shift = speed boost
    camera.setSpeedBoost(k[VK_SHIFT]);

    // Middle mouse click = reset view
    if (edge(window.mouseDown[1], prevKey.mmb)) camera.resetView();

    // Pause
    if (edge(k[VK_SPACE], prevKey.sp)) solarSystem.paused = !solarSystem.paused;

    // Speed
    if (edge(k[VK_OEM_PLUS] || k[VK_ADD], prevKey.eq)) solarSystem.timeScale *= 1.5;
    if (edge(k[VK_OEM_MINUS] || k[VK_SUBTRACT], prevKey.min)) solarSystem.timeScale /= 1.5;

    // Toggles
    if (edge(k['O'], prevKey.o)) showOrbits = !showOrbits;
    if (edge(k['F'], prevKey.f)) fogEnabled = !fogEnabled;
    if (edge(k['Z'], prevKey.z)) zPrepassEnabled = !zPrepassEnabled;
    if (edge(k['B'], prevKey.b)) manualCulling = !manualCulling;
    if (edge(k['V'], prevKey.v)) {
        showDepthViz = !showDepthViz;
        zBuffer.showOverlay = showDepthViz;
        if (showDepthViz) zBuffer.printStats();
    }
    if (edge(k['G'], prevKey.g)) {
        bloomEnabled = !bloomEnabled;
        postProcess.enabled = bloomEnabled;
        printf("[Bloom] %s\n", bloomEnabled ? "ON" : "OFF");
    }
    if (edge(k['A'], prevKey.a)) {
        solarSystem.showAsteroids = !solarSystem.showAsteroids;
        printf("[Asteroids] %s\n", solarSystem.showAsteroids ? "ON" : "OFF");
    }
    if (edge(k['H'], prevKey.h)) {
        shadowEnabled = !shadowEnabled;
        shadowMap.enabled = shadowEnabled;
        printf("[Shadow] %s\n", shadowEnabled ? "ON" : "OFF");
    }

    // Sun intensity mode toggle (L key)
    if (edge(k['L'], prevKey.l)) {
        scrollSunMode = !scrollSunMode;
        printf("[Sun] Scroll controls %s (intensity=%.1f)\n",
               scrollSunMode ? "SUN BRIGHTNESS" : "ZOOM", sunIntensity);
    }

    // Reset time
    if (edge(k['R'], prevKey.r)) {
        solarSystem.simulationTime = 0;
        solarSystem.timeScale = 1.0;
        printf("[Time] Reset\n");
    }

    // ── Material Editor ────────────────────────────────
    if (edge(k['M'], prevKey.m)) {
        matEditor.active = !matEditor.active;
        if (matEditor.active) {
            matEditor.setBodyList(&solarSystem.bodies);
            matEditor.nextBody();
            printf("=== MATERIAL EDITOR ON ===\n");
            printf("  N=next P=prev  Up/Down=adjust  Left/Right=param\n");
            printf("  5=Lava 6=Ice 7=Metal 8=Reset\n");
        } else {
            printf("=== MATERIAL EDITOR OFF ===\n");
        }
    }

    if (matEditor.active) {
        if (edge(k['N'], prevKey.n)) matEditor.nextBody();
        if (edge(k['P'], prevKey.p)) matEditor.prevBody();
        if (edge(k[VK_UP],    prevKey.up)) matEditor.adjust(1.0f);
        if (edge(k[VK_DOWN],  prevKey.dn)) matEditor.adjust(-1.0f);
        if (edge(k[VK_LEFT],  prevKey.lt)) matEditor.prevParam();
        if (edge(k[VK_RIGHT], prevKey.rt)) matEditor.nextParam();
        if (edge(k['5'], prevKey.k5)) matEditor.applyLavaPreset();
        if (edge(k['6'], prevKey.k6)) matEditor.applyIcePreset();
        if (edge(k['7'], prevKey.k7)) matEditor.applyMetalPreset();
        if (edge(k['8'], prevKey.k8)) matEditor.resetToPreset();
    }

    // Free camera movement (continuous, not edge-triggered)
    if (camera.mode == FREE_CAMERA) {
        if (k['W']) camera.moveForward(0.05f);
        if (k['S']) camera.moveForward(-0.05f);
        if (k['A']) camera.moveRight(-0.05f);
        if (k['D']) camera.moveRight(0.05f);
        if (k['E']) camera.moveUp(0.05f);
        if (k['C']) camera.moveUp(-0.05f);
    }

    if (k[VK_ESCAPE]) window.shouldClose = true;
}

// ── HUD helpers ────────────────────────────────────────
static unsigned int hudVAO=0, hudVBO=0;

void initHUD() {
    float quad[]={0,0, 1,0, 0,1, 1,0, 1,1, 0,1};
    glGenVertexArrays(1,&hudVAO); glGenBuffers(1,&hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER,hudVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void drawQuad2D(float x,float y,float w,float h,const glm::vec4& color) {
    float q[]={x, y, x+w, y, x, y+h, x+w, y, x+w, y+h, x, y+h};
    glBindBuffer(GL_ARRAY_BUFFER,hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(q),q);
    glUniform4f(glGetUniformLocation(hudShaderID,"uColor"),color.x,color.y,color.z,color.w);
    glUniform2f(glGetUniformLocation(hudShaderID,"uScreenSize"),(float)window.width,(float)window.height);
    glBindVertexArray(hudVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

// ── Helper: render a single planet body (shading pass) ──
void renderPlanetBody(CelestialBody& body, const glm::mat4& view,
                      const glm::mat4& projection, const glm::vec3& viewPos,
                      bool isSun)
{
    planetShader->use();

    float r = body.getVisualRadius();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, body.worldPos());
    model = glm::rotate(model, (float)body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(r, r, r));

    planetShader->setBool("useDiffuseMap", body.hasTextures && body.diffuseTex != 0);
    planetShader->setBool("useSpecularMap", body.hasTextures && body.specularTex != 0);
    planetShader->setBool("useNormalMap", body.hasTextures && body.normalTex != 0);
    planetShader->setBool("manualCulling", manualCulling);

    if (body.hasTextures && body.diffuseTex != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, body.diffuseTex);
        planetShader->setInt("diffuseMap", 0);
    }
    if (body.hasTextures && body.specularTex != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, body.specularTex);
        planetShader->setInt("specularMap", 1);
    }
    if (body.hasTextures && body.normalTex != 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, body.normalTex);
        planetShader->setInt("normalMap", 2);
    }

    planetShader->setMat4("model", model);
    planetShader->setMat4("view", view);
    planetShader->setMat4("projection", projection);
    planetShader->setVec3("viewPos", viewPos);

    body.material.apply(*planetShader);

    body.sphereMesh.bind();
    glDrawElements(GL_TRIANGLES, body.sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Sun glow halo — multi-layer additive blending
    if (isSun) {
        glowShader->use();
        glowShader->setMat4("view", view);
        glowShader->setMat4("projection", projection);
        glowShader->setVec3("uViewPos", viewPos);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        // Inner tight glow — barely larger than sun disc
        glm::mat4 g1 = glm::mat4(1.0f);
        g1 = glm::translate(g1, body.worldPos());
        g1 = glm::scale(g1, glm::vec3(r * 1.15f));
        glowShader->setMat4("model", g1);
        glowShader->setVec3("uGlowColor", glm::vec3(1.0f, 0.9f, 0.3f));
        glowShader->setFloat("uIntensity", 3.5f);
        body.sphereMesh.bind();
        glDrawElements(GL_TRIANGLES, body.sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

        // Mid glow — warm corona
        glm::mat4 g2 = glm::mat4(1.0f);
        g2 = glm::translate(g2, body.worldPos());
        g2 = glm::scale(g2, glm::vec3(r * 1.35f));
        glowShader->setMat4("model", g2);
        glowShader->setVec3("uGlowColor", glm::vec3(1.0f, 0.65f, 0.12f));
        glowShader->setFloat("uIntensity", 1.8f);
        glDrawElements(GL_TRIANGLES, body.sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

        // Outer soft glow — faint extended halo
        glm::mat4 g3 = glm::mat4(1.0f);
        g3 = glm::translate(g3, body.worldPos());
        g3 = glm::scale(g3, glm::vec3(r * 1.7f));
        glowShader->setMat4("model", g3);
        glowShader->setVec3("uGlowColor", glm::vec3(1.0f, 0.40f, 0.05f));
        glowShader->setFloat("uIntensity", 0.8f);
        glDrawElements(GL_TRIANGLES, body.sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // Saturn rings
    if (body.name == "Saturn") {
        ringShader->use();
        ringShader->setBool("fogEnabled", fogEnabled);
        ringShader->setVec3("fogColor", glm::vec3(0.0f, 0.0f, 0.03f));
        ringShader->setFloat("fogDensity", 0.00002f);
        ringShader->setInt("fogMode", fogModeIdx);
        ringShader->setFloat("fogNear", 1.0f);
        ringShader->setFloat("fogFar", 300.0f);
        ringShader->setVec3("lightPos", solarSystem.sunPosition());
        ringShader->setVec3("viewPos", viewPos);

        glm::mat4 ringModel = glm::mat4(1.0f);
        ringModel = glm::translate(ringModel, body.worldPos());
        ringModel = glm::scale(ringModel, glm::vec3(r * 1.8f));
        ringModel = glm::rotate(ringModel, glm::radians(26.7f), glm::vec3(1.0f, 0.0f, 0.0f));
        ringShader->setMat4("model", ringModel);
        ringShader->setMat4("view", view);
        ringShader->setMat4("projection", projection);
        ringShader->setFloat("alpha", 0.90f);
        ringShader->setVec4("ringColor", glm::vec4(0.85f, 0.78f, 0.60f, 0.90f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        solarSystem.ringTemplate.bind();
        glDrawElements(GL_TRIANGLES, solarSystem.ringTemplate.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }

    // Uranus rings
    if (body.name == "Uranus") {
        ringShader->use();
        ringShader->setBool("fogEnabled", fogEnabled);
        ringShader->setVec3("fogColor", glm::vec3(0.0f, 0.0f, 0.03f));
        ringShader->setFloat("fogDensity", 0.00002f);
        ringShader->setInt("fogMode", fogModeIdx);
        ringShader->setFloat("fogNear", 1.0f);
        ringShader->setFloat("fogFar", 300.0f);
        ringShader->setVec3("lightPos", solarSystem.sunPosition());
        ringShader->setVec3("viewPos", viewPos);

        glm::mat4 ringModel = glm::mat4(1.0f);
        ringModel = glm::translate(ringModel, body.worldPos());
        ringModel = glm::scale(ringModel, glm::vec3(r * 1.55f));
        ringModel = glm::rotate(ringModel, glm::radians(82.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ringShader->setMat4("model", ringModel);
        ringShader->setMat4("view", view);
        ringShader->setMat4("projection", projection);
        ringShader->setFloat("alpha", 0.35f);
        ringShader->setVec4("ringColor", glm::vec4(0.55f, 0.60f, 0.65f, 0.35f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        solarSystem.uranusRingMesh.bind();
        glDrawElements(GL_TRIANGLES, solarSystem.uranusRingMesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
}

// ── Helper: Z-Prepass for a single body ─────────────────
void zPrepassBody(CelestialBody& body, const glm::mat4& view,
                  const glm::mat4& projection)
{
    float r = body.getVisualRadius();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, body.worldPos());
    model = glm::rotate(model, (float)body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(r, r, r));

    zPrepassShader->setMat4("model", model);
    zPrepassShader->setMat4("view", view);
    zPrepassShader->setMat4("projection", projection);

    body.sphereMesh.bind();
    glDrawElements(GL_TRIANGLES, body.sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Saturn rings Z-Prepass
    if (body.name == "Saturn") {
        glm::mat4 ringModel = glm::mat4(1.0f);
        ringModel = glm::translate(ringModel, body.worldPos());
        ringModel = glm::scale(ringModel, glm::vec3(r * 1.8f));
        ringModel = glm::rotate(ringModel, glm::radians(26.7f), glm::vec3(1.0f, 0.0f, 0.0f));
        zPrepassShader->setMat4("model", ringModel);
        solarSystem.ringTemplate.bind();
        glDrawElements(GL_TRIANGLES, solarSystem.ringTemplate.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

// ── Shadow pass: render bodies to depth map ──────────────
void shadowPass() {
    if (!shadowEnabled) return;
    shadowMap.updateLightMatrix(solarSystem.sunPosition());
    shadowMap.beginPass();
    shadowShader->use();

    for (auto& body : solarSystem.bodies) {
        float r = body->getVisualRadius();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, body->worldPos());
        model = glm::rotate(model, (float)body->rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(r, r, r));
        shadowShader->setMat4("model", model);
        shadowShader->setMat4("lightSpaceMatrix", shadowMap.lightSpaceMatrix);
        body->sphereMesh.bind();
        glDrawElements(GL_TRIANGLES, body->sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Saturn ring shadow
        if (body->name == "Saturn") {
            glm::mat4 ringModel = glm::mat4(1.0f);
            ringModel = glm::translate(ringModel, body->worldPos());
            ringModel = glm::scale(ringModel, glm::vec3(r * 1.8f));
            ringModel = glm::rotate(ringModel, glm::radians(26.7f), glm::vec3(1.0f, 0.0f, 0.0f));
            shadowShader->setMat4("model", ringModel);
            solarSystem.ringTemplate.bind();
            glDrawElements(GL_TRIANGLES, solarSystem.ringTemplate.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
    shadowMap.endPass(window.width, window.height);
}

// ── Render ────────────────────────────────────────────
void renderScene() {
    glm::vec3 fogCol(0.0f, 0.0f, 0.03f);

    // Bloom: render scene to FBO
    if (bloomEnabled) {
        glClearColor(fogCol.x, fogCol.y, fogCol.z, 1.0f);
        postProcess.beginScene();
    } else {
        glClearColor(fogCol.x, fogCol.y, fogCol.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    int w = window.width, h = window.height;
    float aspect = (float)w / (float)h;
    glm::mat4 projection = camera.getProjection(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 viewPos = camera.getSmoothPosition();

    auto setupFog = [&](Shader& s) {
        s.setBool("fogEnabled", fogEnabled);
        s.setVec3("fogColor", fogCol);
        s.setFloat("fogDensity", 0.00002f);
        s.setInt("fogMode", fogModeIdx);
        s.setFloat("fogNear", 1.0f);
        s.setFloat("fogFar", 300.0f);
    };

    // 1. Starfield
    starfield.render(*starShader, view, projection, (float)window.getTime());

    // 2. Update lighting + shadow pass
    lights.updateSunPosition(solarSystem.sunPosition());
    // Apply sun intensity multiplier to sun light (index 0)
    lights.pointLights[0].ambient  = glm::vec3(0.8f,0.8f,0.8f) * sunIntensity;
    lights.pointLights[0].diffuse  = glm::vec3(3.5f,3.2f,2.5f) * sunIntensity;
    lights.pointLights[0].specular = glm::vec3(1.8f,1.5f,1.2f) * sunIntensity;
    shadowPass();

    // 3. Z-Prepass
    if (zPrepassEnabled) {
        zPrepassShader->use();
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        for (auto& body : solarSystem.bodies)
            zPrepassBody(*body, view, projection);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
    }

    // 4. Planet shading pass — set global uniforms once
    planetShader->use();
    lights.apply(*planetShader);
    setupFog(*planetShader);
    planetShader->setVec3("viewPos", viewPos);
    planetShader->setFloat("simTime", (float)solarSystem.simulationTime);

    // Shadow uniforms
    planetShader->setBool("shadowEnabled", shadowEnabled);
    planetShader->setMat4("lightSpaceMatrix", shadowMap.lightSpaceMatrix);
    shadowMap.bindDepthTex(3);
    planetShader->setInt("shadowMap", 3);

    // Environment cubemap — globally enabled for all planets
    planetShader->setBool("environmentEnabled", true);
    planetShader->setFloat("environmentIntensity", 0.7f);  // boosted
    envMap.bind(4);
    planetShader->setInt("environmentMap", 4);

    for (auto& body : solarSystem.bodies) {
        int pType = 0;
        if (body->name == "Sun")          pType = 0;
        else if (body->name == "Mercury") pType = 1;
        else if (body->name == "Venus")   pType = 2;
        else if (body->name == "Earth")   pType = 3;
        else if (body->name == "Mars")    pType = 4;
        else if (body->name == "Jupiter") pType = 5;
        else if (body->name == "Saturn")  pType = 6;
        else if (body->name == "Uranus")  pType = 7;
        else if (body->name == "Neptune") pType = 8;
        planetShader->setInt("planetType", pType);

        renderPlanetBody(*body, view, projection, viewPos, body->name == "Sun");
    }

    // Restore depth state
    if (zPrepassEnabled) {
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }

    // 4b. Moon
    if (solarSystem.moon) {
        planetShader->use();
        planetShader->setInt("planetType", 9);
        float moonScale = 0.08f;
        glm::mat4 moonModel = glm::mat4(1.0f);
        glm::vec3 moonWorld = solarSystem.earthWorldPos() +
            glm::vec3((float)solarSystem.moon->position.x * CelestialBody::ORBIT_SCALE * 0.05,
                      (float)solarSystem.moon->position.y * CelestialBody::ORBIT_SCALE * 0.05,
                      (float)solarSystem.moon->position.z * CelestialBody::ORBIT_SCALE * 0.05);
        moonModel = glm::translate(moonModel, moonWorld);
        moonModel = glm::scale(moonModel, glm::vec3(moonScale));

        planetShader->setBool("useDiffuseMap", true);
        planetShader->setBool("useSpecularMap", false);
        planetShader->setBool("useNormalMap", solarSystem.moon->normalTex != 0);
        planetShader->setBool("manualCulling", manualCulling);
        solarSystem.moon->material.apply(*planetShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, solarSystem.moon->diffuseTex);
        planetShader->setInt("diffuseMap", 0);
        if (solarSystem.moon->normalTex) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, solarSystem.moon->normalTex);
            planetShader->setInt("normalMap", 2);
        }

        planetShader->setMat4("model", moonModel);
        planetShader->setMat4("view", view);
        planetShader->setMat4("projection", projection);
        planetShader->setVec3("viewPos", viewPos);

        solarSystem.moon->sphereMesh.bind();
        glDrawElements(GL_TRIANGLES, solarSystem.moon->sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // 4c. Asteroid belt
    if (solarSystem.showAsteroids && solarSystem.asteroidMesh.VAO) {
        solarSystem.updateAsteroidBuffer();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glPointSize(2.5f);
        orbitShader->use();
        orbitShader->setVec3("uColor", glm::vec3(0.55f, 0.42f, 0.30f));
        glm::mat4 astModel = glm::mat4(1.0f);
        orbitShader->setMat4("model", astModel);
        orbitShader->setMat4("view", view);
        orbitShader->setMat4("projection", projection);
        solarSystem.asteroidMesh.bind();
        glDrawArrays(GL_POINTS, 0, solarSystem.asteroidMesh.indexCount);
        glBindVertexArray(0);
    }

    // 5. Orbits
    if (showOrbits) {
        orbitShader->use();
        orbitShader->setVec3("uColor", glm::vec3(0.3f, 0.35f, 0.5f));
        for (auto& body : solarSystem.bodies) {
            if (body->orbit.semiMajorAxis > 0) {
                glm::mat4 orbitModel = glm::mat4(1.0f);
                orbitShader->setMat4("model", orbitModel);
                orbitShader->setMat4("view", view);
                orbitShader->setMat4("projection", projection);
                body->orbitLine.bind();
                glDrawArrays(GL_LINE_LOOP, 0, body->orbitLine.indexCount);
                glBindVertexArray(0);
            }
        }
    }

    // 6. Bloom composite
    if (bloomEnabled) {
        postProcess.endScene(bloomExtractID, bloomBlurID, bloomCompositeID);
    }

    // 7. HUD
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(hudShaderID);

    drawQuad2D(10, (float)h - 180, 310, 170, glm::vec4(0.0f, 0.02f, 0.08f, 0.65f));
    drawQuad2D(10, (float)h - 10,  310, 1.5f, glm::vec4(0.25f, 0.45f, 0.75f, 0.7f));
    drawQuad2D(10, (float)h - 180, 1.5f, 170, glm::vec4(0.25f, 0.45f, 0.75f, 0.7f));
    drawQuad2D(320, (float)h - 180, 1.5f, 170, glm::vec4(0.25f, 0.45f, 0.75f, 0.7f));
    drawQuad2D(10, (float)h - 180, 310, 1.5f, glm::vec4(0.25f, 0.45f, 0.75f, 0.7f));

    // Material editor HUD panel (bright orange border for visibility)
    if (matEditor.active && matEditor.targetBody) {
        float panelY = (float)h - 380;
        drawQuad2D(10, panelY, 530, 190, glm::vec4(0.08f, 0.04f, 0.0f, 0.85f));
        drawQuad2D(10, panelY + 188, 530, 2.5f, glm::vec4(1.0f, 0.45f, 0.0f, 1.0f));
        drawQuad2D(10, panelY, 2.5f, 190, glm::vec4(1.0f, 0.45f, 0.0f, 1.0f));
        drawQuad2D(538, panelY, 2.5f, 190, glm::vec4(1.0f, 0.45f, 0.0f, 1.0f));
        drawQuad2D(10, panelY, 530, 2.5f, glm::vec4(1.0f, 0.45f, 0.0f, 1.0f));
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(0);

    // 8. Depth visualization
    if (showDepthViz) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        zBuffer.renderOverlay(depthVizShaderID);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    // Window title
    char title[512];
    if (matEditor.active && matEditor.targetBody) {
        Material& em = matEditor.targetBody->material;
        snprintf(title, sizeof(title),
            "EDIT [%s] Param:%s | R=%.2f M=%.2f A=%.2f SS=%.2f CC=%.2f E=(%.1f,%.1f,%.1f) | 5/6/7=Preset 8=Reset",
            matEditor.targetBody->name.c_str(),
            MaterialEditor::paramName((MaterialEditor::Param)matEditor.currentParam),
            em.roughness, em.metallic, em.anisotropy,
            em.subsurface, em.clearcoat,
            em.emissive.x, em.emissive.y, em.emissive.z);
    } else {
        snprintf(title, sizeof(title),
            "Solar System 3D | %s @ %s | FPS:%d | Sun:%.1f %s | %s | Z:%s S:%s G:%s | [L=SunAdj Tab=Target]",
            camera.modeName(), camera.targetName.c_str(),
            fpsCounter.getFPS(),
            sunIntensity, scrollSunMode ? "(SCROLL)" : "",
            solarSystem.paused ? "PAUSED" : "RUNNING",
            zPrepassEnabled ? "ZP" : "--",
            shadowEnabled ? "SD" : "--",
            bloomEnabled ? "BL" : "--");
    }
    window.setTitle(title);
    window.swapBuffers();
}

// ── Main ──────────────────────────────────────────────
int main() {
    AllocConsole();
    freopen("CONOUT$","w",stdout);
    freopen("CONOUT$","w",stderr);
    SetConsoleOutputCP(65001);  // UTF-8 for Chinese character support
    printf("=== Solar System 3D — Multi-Material PBR Renderer ===\n");

    if (!window.create("Solar System 3D",1280,720,true)) {
        MessageBoxA(nullptr,"Failed to create OpenGL window","Fatal Error",MB_OK|MB_ICONERROR);
        return -1;
    }

    printf("OpenGL : %s\n",glGetString(GL_VERSION));
    printf("GLSL   : %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("GPU    : %s\n",glGetString(GL_RENDERER));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_MULTISAMPLE);

    window.onResize=[](int w,int h){
        glViewport(0,0,w,h); zBuffer.init(w,h); postProcess.init(w,h);
    };
    window.onScroll=[](float d){
        if (scrollSunMode) {
            sunIntensity += d * 0.15f;
            sunIntensity = glm::clamp(sunIntensity, 0.1f, 5.0f);
            printf("[Sun] Intensity: %.1f\n", sunIntensity);
        } else {
            camera.processScroll(d);
        }
    };
    window.onMouseDrag=[](float dx,float dy){
        // Left button = rotate, Middle button = pan
        if (window.mouseDown[1])
            camera.processPan(dx, dy);
        else if (window.mouseDown[0])
            camera.processMouseDrag(dx, dy);
    };

    // Load shaders
    planetShader   = new Shader("shaders/planet.vert", "shaders/planet.frag");
    ringShader     = new Shader("shaders/ring.vert", "shaders/ring.frag");
    starShader     = new Shader("shaders/starfield.vert", "shaders/starfield.frag");
    glowShader     = new Shader("shaders/glow.vert", "shaders/glow.frag");
    zPrepassShader = new Shader("shaders/zprepass.vert", "shaders/zprepass.frag");
    shadowShader   = new Shader("shaders/shadow.vert", "shaders/shadow.frag");
    Shader hudShader("shaders/hud.vert", "shaders/hud.frag");
    hudShaderID = hudShader.ID;

    Shader tmpOrbit; tmpOrbit.ID=compileInlineShader(orbitVertSrc,orbitFragSrc);
    orbitShader=new Shader(tmpOrbit);

    Shader tmpDepthViz; tmpDepthViz.ID=compileInlineShader(depthVizVertSrc,depthVizFragSrc);
    depthVizShaderID=tmpDepthViz.ID;

    Shader tmpBloom; tmpBloom.ID=compileInlineShader(postVertSrc, bloomExtractFragSrc);
    bloomExtractID=tmpBloom.ID;
    tmpBloom.ID=compileInlineShader(postVertSrc, bloomBlurFragSrc); bloomBlurID=tmpBloom.ID;
    tmpBloom.ID=compileInlineShader(postVertSrc, bloomCompositeFragSrc); bloomCompositeID=tmpBloom.ID;

    printf("Planet:%u Ring:%u Star:%u Glow:%u ZPrep:%u Shadow:%u HUD:%u Orbit:%u\n",
           planetShader->ID, ringShader->ID, starShader->ID, glowShader->ID,
           zPrepassShader->ID, shadowShader->ID, hudShaderID, orbitShader->ID);

    // Init scene
    solarSystem.initialize();
    starfield.initialize();
    zBuffer.init(window.width, window.height);
    zBuffer.showOverlay = false;
    postProcess.init(window.width, window.height);
    postProcess.enabled = bloomEnabled;
    shadowMap.init(2048);
    envMap.init(256);
    initHUD();

    // Debug: verify planet textures
    printf("\n--- Planet textures ---\n");
    for (auto& b : solarSystem.bodies) {
        printf("  %-8s: diff=%u spec=%u norm=%u hasTex=%d r=%.2f material=%s\n",
               b->name.c_str(), b->diffuseTex, b->specularTex, b->normalTex,
               b->hasTextures, b->getVisualRadius(), b->material.presetName);
    }
    printf("--- end ---\n\n");

    // Sun — bright distant point light (near-directional behavior)
    lights.useDirLight=false;
    PointLight sunLight;
    sunLight.position=glm::vec3(0,0,0);
    sunLight.ambient=glm::vec3(0.8f,0.8f,0.8f);   // boosted ambient
    sunLight.diffuse=glm::vec3(3.5f,3.2f,2.5f);    // boosted diffuse
    sunLight.specular=glm::vec3(1.8f,1.5f,1.2f);
    sunLight.constant=1.0f; sunLight.linear=0.00005f; sunLight.quadratic=0.000005f;  // much slower falloff
    lights.pointLights.push_back(sunLight);

    // Dynamic orbit light
    PointLight orbitLight;
    orbitLight.ambient=glm::vec3(0.05f,0.05f,0.1f);
    orbitLight.diffuse=glm::vec3(0.3f,0.5f,1.0f);
    orbitLight.specular=glm::vec3(0.3f,0.4f,0.6f);
    orbitLight.constant=1.0f; orbitLight.linear=0.05f; orbitLight.quadratic=0.01f;
    lights.pointLights.push_back(orbitLight);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           太阳系多材质PBR渲染系统 — 按键功能表              ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║                                                              ║\n");
    printf("║  【相机视角切换】                                            ║\n");
    printf("║    1  —— 自由相机模式 (Free Camera)  WASD飞行 鼠标旋转     ║\n");
    printf("║    2  —— 轨道相机模式 (Orbit Camera) 围绕天体旋转          ║\n");
    printf("║    3  —— 俯视全局模式 (Top-Down View) 从上方俯瞰太阳系    ║\n");
    printf("║                                                              ║\n");
    printf("║  【视角操控】                                                ║\n");
    printf("║    Tab       —— 切换轨道目标天体 (太阳→水星→金星→...)     ║\n");
    printf("║    鼠标左键拖拽 —— 旋转视角                                ║\n");
    printf("║    鼠标中键拖拽 —— 平移视角                                ║\n");
    printf("║    鼠标中键单击 —— 重置当前视角                            ║\n");
    printf("║    鼠标滚轮     —— 拉近/拉远                                ║\n");
    printf("║    Shift 按住  —— 自由模式下加速移动 (3.5倍速)             ║\n");
    printf("║    WASD        —— 自由模式下前后左右移动                   ║\n");
    printf("║    E / C       —— 自由模式下上升/下降                      ║\n");
    printf("║                                                              ║\n");
    printf("║  【系统控制】                                                ║\n");
    printf("║    空格键 —— 暂停/恢复时间流逝                             ║\n");
    printf("║    +/-    —— 加速/减速时间倍率                             ║\n");
    printf("║    R      —— 重置模拟时间                                  ║\n");
    printf("║    ESC    —— 退出程序                                      ║\n");
    printf("║                                                              ║\n");
    printf("║  【显示开关】                                                ║\n");
    printf("║    O —— 显示/隐藏轨道线                                    ║\n");
    printf("║    F —— 开启/关闭雾效                                     ║\n");
    printf("║    L —— 切换滚轮模式 (缩放 / 太阳亮度调节)                ║\n");
    printf("║    H —— 开启/关闭阴影映射                                 ║\n");
    printf("║    G —— 开启/关闭Bloom泛光后处理                          ║\n");
    printf("║    Z —— 开启/关闭Z-Prepass深度预剔除                      ║\n");
    printf("║    A —— 显示/隐藏小行星带                                 ║\n");
    printf("║    B —— 手动/硬件背面剔除切换                             ║\n");
    printf("║    V —— 深度可视化覆盖层                                  ║\n");
    printf("║                                                              ║\n");
    printf("║  【材质编辑器】 (M键开启)                                   ║\n");
    printf("║    M      —— 开启/关闭材质编辑器                           ║\n");
    printf("║    N / P  —— 选择下一个/上一个天体                         ║\n");
    printf("║    ↑ ↓    —— 增大/减小当前材质参数                        ║\n");
    printf("║    ← →    —— 切换上一个/下一个参数项                      ║\n");
    printf("║    5      —— 应用熔岩(Lava)材质预设                       ║\n");
    printf("║    6      —— 应用冰晶(Ice Crystal)材质预设                ║\n");
    printf("║    7      —— 应用金属小行星(Metallic)材质预设             ║\n");
    printf("║    8      —— 重置天体为默认材质                           ║\n");
    printf("║                                                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    double lastTime=window.getTime();

    while (window.pollEvents()) {
        processInput();
        fpsCounter.update(window.getTime());

        double now=window.getTime();
        double dt=now-lastTime;
        lastTime=now;
        if (dt>0.1) dt=0.1;
        solarSystem.update(dt);

        // Animate dynamic orbit light
        float lightAngle = (float)solarSystem.simulationTime * 4.0f;
        float lightDist = 3.0f * CelestialBody::ORBIT_SCALE;
        lights.pointLights[1].position = glm::vec3(
            cos(lightAngle) * lightDist, 0.5f * CelestialBody::ORBIT_SCALE,
            sin(lightAngle) * lightDist);

        camera.updateTransition((float)dt);
        renderScene();
        zBuffer.update();
    }

    delete planetShader; delete ringShader; delete starShader; delete orbitShader;
    delete glowShader; delete zPrepassShader; delete shadowShader;
    postProcess.cleanup();
    zBuffer.cleanup();
    shadowMap.cleanup();
    envMap.cleanup();
    window.destroy();
    return 0;
}
