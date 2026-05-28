# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```
cmake -B build -S .
cmake --build build
```

The executable is `build/solar_system.exe` (or `build/<config>/solar_system.exe` for multi-config generators like VS). CMake copies shaders into the output directory automatically. The only external dependency is GLAD (bundled in `third_party/`).

**Requirements**: OpenGL 3.3 Core, Windows (Win32 + WGL), C++17, CMake 3.20+.

## Architecture

This is a single-file-application OpenGL solar system simulation. Every subsystem is a header-only class under `src/`; `main.cpp` (~965 lines) wires them together and holds the render loop.

### Layering

| Layer | Files | Purpose |
|-------|-------|---------|
| Core | `src/core/` | Win32/WGL window, OpenGL shader helper, camera (3 modes), mesh factories, self-contained `glm` math library |
| Rendering | `src/rendering/` | PBR material system (13 presets), shadow map, bloom post-process (bright-pass → separable blur → composite), procedural cubemap environment map, Z-prepass, CPU Z-buffer visualization, runtime material editor |
| Scene | `src/scene/` | `CelestialBody` (Keplerian orbit solver via Newton's method), `SolarSystem` (planet init + asteroid belt), `Starfield` |
| Utils | `src/utils/` | FPS counter, procedural texture generator (Perlin/Voronoi/FBM/domain warp — generates all planet textures at startup) |
| Entry | `main.cpp` | Window creation, shader loading, input handling, render loop |

### Key design decisions

- **No GLM dependency**: `src/core/MglMath.h` implements `glm::vec2/3/4`, `glm::mat4`, and all transform/projection functions directly.
- **No external texture files**: `TextureGenerator.h` generates all diffuse/normal/specular maps procedurally at startup via GL texture creation.
- **Inline shaders**: Bloom and HUD shaders are compiled from inline GLSL strings in `main.cpp`; planet/star/ring/orbit shaders come from files in `shaders/`.
- **Z-Prepass**: An optional depth-only first pass (zprepass shader) followed by the shading pass with `glDepthFunc(GL_LEQUAL)` and `glDepthMask(GL_FALSE)` to reduce overdraw.
- **Material system**: `Material` struct holds full PBR parameters (roughness, metallic, anisotropy, subsurface, clearcoat) applied per-planet. `MaterialEditor` (M key) allows runtime tweaking.

### Render pipeline (per frame)

1. Starfield background
2. Shadow map pass (directional light depth map from sun position)
3. Optional Z-prepass (depth-only)
4. Planet shading pass with shadow sampling + environment cubemap
5. Moon rendering (orbiting Earth, separate model matrix)
6. Asteroid belt (GL_POINTS, dynamic VBO update)
7. Orbit lines (GL_LINE_LOOP)
8. Post-process bloom composite (if enabled)
9. HUD overlay
10. Optional depth visualization overlay

### Camera modes

- `FREE_CAMERA` (key 1): WASD fly, mouse drag rotate, scroll zoom
- `ORBIT_CAMERA` (key 2): pivot around a selected body, Tab cycles targets
- `TOP_DOWN_VIEW` (key 3): bird's-eye view with adjustable height and angle

### Orbital mechanics

`CelestialBody::updatePosition()` solves Kepler's equation (`M = E - e·sin(E)`) via Newton's method to compute position in AU from orbital elements. `CelestialBody::toDisplayRadius(au)` applies a non-linear power function (`pow(au, 0.55) * 17.0`) to compress the 78× real-orbital-range into a well-proportioned view where inner planets clear the Sun disc and outer planets fit within the far plane. All planet data (real masses, radii, orbital elements) is hardcoded in `SolarSystem::initialize()`.
