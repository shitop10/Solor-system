# 太阳系三维渲染系统 — 进阶优化方案

---

## 当前状态诊断

经过对全部源码（`main.cpp`、10个shader、13个头文件）的详细审查，当前系统存在以下核心问题：

### 1. 光照系统过于简陋
- `planet.frag` 中的 `calcLight()` 仅为**简化的 Blinn-Phong**，没有完整 Phong 模型
- 仅有**单个点光源**（太阳），`Light.h` 中 `LightManager` 虽支持多光源但从未使用
- 衰减模型粗糙（`constant/linear/quadratic` 参数不合理）
- 没有镜面高光的正确计算，粗糙度硬编码在 shader 中

### 2. 材质系统形同虚设
- `Material.h` 虽然定义了完整 PBR 材质参数，但 **shader 并未使用它们**
- `planet.frag` 完全绕过 `material` uniform，用硬编码的 `rough` 值
- 所有行星共用同一个光照函数，没有材质差异化
- 没有金属/玻璃/木头/磨砂等物理材质区分

### 3. 纹理系统未充分利用
- `TextureGenerator.h` 生成了高质量的 diffuse/normal/specular 贴图
- 但 `planet.frag` **完全不用纹理采样**，而是内置了 `sunSurface()`/`earthSurface()` 等过程函数
- 法线贴图仅仅为 Earth 生成，shader 中没有法线映射代码
- 高光贴图为 Earth 生成但 shader 从未采样

### 4. 消隐仅依赖硬件
- 只用了 `glEnable(GL_DEPTH_TEST)` + `GL_CULL_FACE`
- 没有手写 Z-Buffer 深度算法
- 没有软件级背面剔除

### 5. 画面质量不足
- `GL_MULTISAMPLE` 启用但窗口创建时未配置 MSAA（等于无效抗锯齿）
- 雾效仅有简单指数雾
- 行星渲染粗糙——没有实际纹理贴图，仅靠 shader 过程噪声
- 土星环缺少纹理细节
- 没有后处理效果（bloom、HDR、色调映射）
- 星场是简单点精灵，质量较低

### 6. 缺失的重要天文特征
- 天王星也有环，未实现
- 没有小行星带
- 没有月球
- 太阳日冕效果简陋

---

## 优化方案（分 6 个阶段，由底层到上层）

---

## 阶段一：深度缓冲与消隐优化

### 1.1 手写 Z-Buffer 算法
**目标**：不依赖 `GL_DEPTH_TEST`，在 shader 中手动维护深度缓冲，理解 GPU 深度测试原理。

**实现方案**：
- 新建 `shaders/zpass.vert` / `shaders/zpass.frag`，实现两遍渲染（Two-Pass Z-Prepass）：
  - **第一遍（Z-Pass）**：只写深度，不计算颜色
  - **第二遍（Shading Pass）**：读取深度缓冲进行 Early-Z 比较
- 在 `planet.frag` 中添加手写深度比较逻辑，使用 `gl_FragDepth` 输出深度值
- 新建 `src/rendering/ZBuffer.h`，CPU 端软件 Z-Buffer 实现（用于教学对比）
  - 在单独窗口中可视化深度图

**涉及文件**：
- 新建：`shaders/zprepass.vert`、`shaders/zprepass.frag`
- 修改：`shaders/planet.frag`（添加手动深度比较）
- 新建：`src/rendering/ZBuffer.h`

### 1.2 手写背面剔除
**目标**：在顶点着色器中通过法线方向手动剔除背面三角形，理解 CullFace 原理。

**实现方案**：
- 在 `planet.vert` 中计算三角形法线与视线方向的点积
- 当 `dot(faceNormal, viewDir) > 0` 时（面向相机的面），将顶点移到裁剪空间外（`gl_Position.w = 0`）实现剔除
- 添加 uniform `bool manualCulling` 开关，可与硬件 `GL_CULL_FACE` 对比效果

**涉及文件**：
- 修改：`shaders/planet.vert`
- 修改：`main.cpp`（添加快捷键切换手动/硬件剔除）

---

## 阶段二：高级光照系统（Phong 模型 + 多光源）

### 2.1 完整 Phong 光照模型
**目标**：实现标准 Phong 光照（Ambient + Diffuse + Specular），替代当前的简化 Blinn-Phong。

**实现方案**：
- 重写 `planet.frag` 中的光照计算函数：
  ```
  Phong = K_a * I_a  +  K_d * I_d * (N·L)  +  K_s * I_s * (R·V)^α
  ```
  - `K_a/d/s`：材质属性（从 `Material.h` 读取）
  - `R = 2*(N·L)*N - L`：光线反射向量
  - `α`：shininess（高光指数）
- 新增方向光 + 点光源混合计算
- 添加 `Phong` vs `Blinn-Phong` 实时切换（用于对比展示）

**涉及文件**：
- 重写：`shaders/planet.frag`（光照部分完全重构）
- 修改：`shaders/planet.vert`（传递切线空间矩阵给 FS）
- 修改：`main.cpp`（光照参数调整快捷键）

### 2.2 多光源混合与动态光源
**目标**：支持多光源场景，光源可随时间移动。

**实现方案**：
- 当前 `LightManager` 已支持多点光源，在 shader 中实现真正的多光源循环累加
- 添加**轨道光源**：一个发光的"小卫星"绕场景飞行，产生动态阴影效果
- 添加**颜色光源**：在行星附近放置带颜色的点光源（模拟科幻效果）
- 光源衰减适配：根据场景尺度调整 `constant/linear/quadratic` 参数
- 光源可视化：为每个动态光源绘制发光线框球

**涉及文件**：
- 修改：`shaders/planet.frag`（多光源循环）
- 修改：`src/rendering/Light.h`（添加动态光源更新逻辑）
- 修改：`main.cpp`（添加光源实体、光源显示开关）

### 2.3 光照衰减曲线优化
**目标**：使用更真实的衰减函数。

**实现方案**：
- 实现 `UE4 风格衰减`：`att = 1.0 / (1.0 + 2.0*d/R + (d/R)^2)`，其中 R 为光源影响半径
- 添加可视化衰减范围的半透明球
- 提供三种衰减模型切换：标准、UE4、线性

---

## 阶段三：物理材质系统

### 3.1 Shader 端材质重构
**目标**：让 shader 真正读取和使用 `Material` uniform。

**实现方案**：
- 将 `Material` 数据完整传入 shader（`material.ambient/diffuse/specular/emissive/shininess/roughness/metallic`）
- 在 `calcLight()` 中按材质参数计算：

```
// 金属材质
金属: specular → tinted by albedo, diffuse → darkened, roughness → low
玻璃: specular → high, emissive → slight, roughness → very low, alpha < 1
木头: specular → very low, roughness → high, metallic → 0
磨砂: roughness → 0.5~0.7, metallic → 0, specular → medium
```

**涉及文件**：
- 修改：`shaders/planet.frag`（完整材质系统）
- 修改：`src/rendering/Material.h`（扩展材质预设）

### 3.2 各行星材质预设

| 行星 | 材质类型 | 特点 |
|------|----------|------|
| Sun | 自发光 | emissive=high, roughness=0.2, metallic=0 |
| Mercury | 磨砂岩石 | roughness=0.9, metallic=0.05, specular=low |
| Venus | 云层 | roughness=0.7, specular=medium, 微自发光 |
| Earth | 混合(海洋+陆地) | roughness=0.5, specular=0.4 (海洋高光) |
| Mars | 粗糙岩石 | roughness=0.85, metallic=0.1, 微红色金属感 |
| Jupiter | 气体云 | roughness=0.8, specular=very low |
| Saturn | 气体 | roughness=0.8, specular=very low |
| Uranus | 冰质 | roughness=0.35, specular=0.25 (冰反射) |
| Neptune | 冰质 | roughness=0.4, specular=0.2 |

---

## 阶段四：多层纹理贴图系统

### 4.1 真正的纹理采样管线
**目标**：让 shader 实际使用 CPU 端生成的纹理贴图。

**实现方案**：
- **重构 `planet.frag`**：移除硬编码的行星表面函数（`sunSurface/earthSurface` 等），改为 `texture(diffuseMap, uv)` 采样
- CPU 端已生成高质量纹理（`TextureGenerator.h`），直接连线即可
- 保留 GPU 端过程噪声作为**备选方案**（uniform `bool useProceduralTexture`）

**涉及文件**：
- 重写：`shaders/planet.frag`
- 修改：`main.cpp`（确保纹理绑定正确）

### 4.2 法线凹凸映射
**目标**：使用法线贴图为行星表面增加几何细节。

**实现方案**：
- 在 `planet.vert` 中构建 TBN 矩阵：
  ```
  T = normalize(mat3(model) * aTangent)
  B = normalize(mat3(model) * aBitangent)
  N = normalize(mat3(model) * aNormal)
  TBN = mat3(T, B, N)
  ```
- 在 `planet.frag` 中采样法线贴图并转换：
  ```
  vec3 mappedNormal = texture(normalMap, uv).rgb * 2.0 - 1.0;
  vec3 worldNormal = normalize(TBN * mappedNormal);
  ```
- `Mesh.h` 的 `Vertex` 需增加 `tangent` 和 `bitangent` 属性
- `Mesh::createSphere()` 需计算切线向量
- 为所有岩石行星（Mercury, Venus, Mars）和地球生成法线贴图

**涉及文件**：
- 修改：`src/core/Mesh.h`（Vertex 增加 tangent/bitangent，createSphere 计算切线）
- 修改：`shaders/planet.vert`（TBN 矩阵输出）
- 修改：`shaders/planet.frag`（法线映射计算）
- 修改：`src/scene/SolarSystem.h`（为更多行星生成法线贴图）
- 修改：`src/utils/TextureGenerator.h`（增加岩石表面法线生成器）

### 4.3 高光贴图
**目标**：通过高光贴图控制行星表面不同区域的反射强度。

**实现方案**：
- 为 Earth 生成高光贴图：海洋区域高亮（高光），陆地区域暗（低高光）
- 为冰行星（Uranus, Neptune）生成高光贴图
- Shader 中：`float specMask = texture(specularMap, uv).r;`
- 最终高光 = `K_s * I_s * (R·V)^α * specMask`

### 4.4 自发光贴图（Emissive Map）
**目标**：行星暗面城市灯光、火山熔岩等效果。

**实现方案**：
- 为 Earth 生成自发光贴图：模拟城市灯光分布
- 为 Jupiter 生成自发光贴图：模拟闪电/极光
- Shader 中在环境光计算后叠加 emissive 贴图

---

## 阶段五：画面优化与特效

### 5.1 真正的 MSAA 抗锯齿
**目标**：修复当前无效的 `GL_MULTISAMPLE`。

**实现方案**：
- 在 `Window.h` 的 WGL 上下文创建时，配置多重采样属性：
  ```
  WGL_SAMPLE_BUFFERS_ARB = 1
  WGL_SAMPLES_ARB = 4
  ```
- 确保像素格式支持 MSAA
- 同时在 `main.cpp` 中保留 `glEnable(GL_MULTISAMPLE)`

**涉及文件**：
- 修改：`src/core/Window.h`（WGL pixel format 增加 MSAA 属性）

### 5.2 增强雾化特效
**目标**：从简单指数雾升级为多层次雾。

**实现方案**：
- 实现三种雾模型（可切换）：
  1. **线性雾**：`f = (far-d)/(far-near)`
  2. **指数雾**：`f = e^(-density*d)`（当前已有）
  3. **多层雾**：近处淡薄 + 远处浓重
- 为雾添加颜色渐变（从深蓝到浅蓝到黑）
- 为行星大气添加局部"大气散射"效果（Rim Light 增强）

**涉及文件**：
- 修改：`shaders/planet.frag`（增强雾函数）
- 修改：`main.cpp`（雾参数调整）

### 5.3 后处理 — Bloom 辉光
**目标**：让亮部产生辉光扩散效果（太阳、镜面高光）。

**实现方案**：
- 使用离屏渲染帧缓冲（FBO）+ 两遍高斯模糊：
  1. 渲染场景到 FBO
  2. 提取亮部（亮度 > 阈值）
  3. 水平 + 垂直高斯模糊（两遍）
  4. 叠加到原始场景
- 新建 `shaders/bloom.vert` / `shaders/bloom.frag`

**涉及文件**：
- 新建：`shaders/bloom.vert`、`shaders/bloom.frag`、`shaders/blur.vert`、`shaders/blur.frag`
- 新建：`src/rendering/PostProcess.h`
- 修改：`main.cpp`（后处理管线）

### 5.4 帧率监控增强
**目标**：更详细的性能分析显示。

**实现方案**：
- 扩展现有 `FPSCounter.h`：
  - 添加 99th 百分位帧时间
  - 添加最小/最大帧时间
  - 颜色编码（绿 <16ms，黄 16-33ms，红 >33ms）
- 在 HUD 中绘制帧时间直方图（可选）

---

## 阶段六：天文特征增强

### 6.1 土星环纹理化
**目标**：让土星环使用实际纹理贴图而非纯色带。

**实现方案**：
- 在 `TextureGenerator.h` 中添加 `generateRingTexture()` 方法
- 生成环纹理：A环/B环/C环/卡西尼缝的放射状纹理
- 修改 `ring.frag` 支持纹理采样
- 使用 alpha 通道处理环的透明度变化

### 6.2 天王星环
**目标**：为天王星添加薄环。

**实现方案**：
- 复用 `Mesh::createRing()`，以更小的尺寸和更高的透明度
- 天王星环几乎垂直于黄道面（倾斜约98°），需正确设置旋转
- 使用淡灰色半透明纹理

### 6.3 小行星带
**目标**：在火星和木星之间添加小行星带。

**实现方案**：
- 生成 2000-5000 个随机分布的小粒子
- 在 `src/scene/SolarSystem.h` 中添加 `AsteroidBelt` 实体
- 使用实例化渲染（`glDrawArraysInstanced`）提升性能
- 粒子大小随机，颜色灰褐色

### 6.4 地球月球
**目标**：为地球添加月球卫星。

**实现方案**：
- 新增小型 `CelestialBody`：月球
- 月球绕地球运行的轨道参数
- 月球使用灰色岩石纹理
- 月球的轨道以地球当前位置为中心动态更新

---

## 实施优先级建议

```
优先级 1 (核心基础，最优先):
├── 阶段二 — 完整 Phong 光照模型
├── 阶段四 — 纹理采样管线（连线 CPU→GPU）
└── 阶段一 — Z-Prepass 深度优化

优先级 2 (效果提升):
├── 阶段三 — 物理材质系统
├── 阶段四 — 法线映射 + 高光贴图
└── 阶段五 — MSAA + 增强雾效

优先级 3 (锦上添花):
├── 阶段五 — Bloom 后处理
├── 阶段六 — 土星环纹理 + 天王星环 + 小行星带
└── 阶段六 — 月球

优先级 4 (教学展示):
├── 阶段一 — CPU 软件 Z-Buffer 可视化
├── 阶段一 — 手动背面剔除开关
└── 阶段二 — 光照模型切换对比
```

---

## 文件变更汇总

| 操作 | 文件 |
|------|------|
| **重写** | `shaders/planet.frag`（最大改动：纹理采样+完整光照+材质+法线映射） |
| **修改** | `shaders/planet.vert`（TBN 矩阵、手动剔除） |
| **修改** | `shaders/ring.frag`（纹理采样、改进光照） |
| **修改** | `src/core/Mesh.h`（Vertex 增加 tangent/bitangent） |
| **修改** | `src/core/Window.h`（MSAA WGL 配置） |
| **修改** | `src/rendering/Material.h`（扩展材质预设） |
| **修改** | `src/scene/SolarSystem.h`（法线贴图生成、小行星带、月球） |
| **修改** | `src/utils/TextureGenerator.h`（环纹理、岩石法线贴图） |
| **修改** | `main.cpp`（后处理管线、多光源、快捷键、纹理绑定） |
| **新建** | `shaders/zprepass.vert`、`shaders/zprepass.frag` |
| **新建** | `shaders/bloom.vert`、`shaders/bloom.frag` |
| **新建** | `shaders/blur.vert`、`shaders/blur.frag` |
| **新建** | `src/rendering/ZBuffer.h` |
| **新建** | `src/rendering/PostProcess.h` |

---

## 关键技术指标

| 指标 | 当前状态 | 目标 |
|------|----------|------|
| 光照模型 | 简化Blinn-Phong | 完整Phong + 多光源 |
| 纹理使用 | 无（纯过程噪声） | diffuse+normal+specular采样 |
| 材质系统 | Material.h 未被shader使用 | 完整物理材质管线 |
| 抗锯齿 | 假MSAA（窗口未配置） | 4x MSAA |
| 消隐 | 仅硬件深度测试 | Z-Prepass + 手动背面剔除 |
| 特效 | 简单指数雾 | 多层雾+Bloom辉光 |
| 帧率监控 | 简单FPS显示 | 带百分位的详细监控 |
| 天文特征 | 8行星+太阳+土星环 | +天王星环+小行星带+月球 |
