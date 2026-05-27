#version 330 core
// ============================================================
//  Solar System — Multi-Material PBR Fragment Shader
//  Features: anisotropic GGX, subsurface scattering approx,
//            clearcoat layer, shadow mapping, normal mapping,
//            Cook-Torrance PBR, multi-light, fog, rim light
// ============================================================
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 WorldNormal;
in vec3 WorldPos;
in mat3 TBN;
in vec3 WorldTangent;
in vec3 WorldBitangent;
out vec4 FragColor;

// ── Planet identity ────────────────────────────────────
uniform int planetType;
uniform float simTime;

// ── Camera ─────────────────────────────────────────────
uniform vec3 viewPos;
uniform bool manualCulling;

// ── Textures ───────────────────────────────────────────
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;
uniform bool useDiffuseMap;
uniform bool useSpecularMap;
uniform bool useNormalMap;

// ── Material (PBR) ─────────────────────────────────────
uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform vec3 material_emissive;
uniform float material_shininess;
uniform float material_roughness;
uniform float material_metallic;
uniform float material_anisotropy;
uniform float material_subsurface;
uniform float material_clearcoat;
uniform float material_clearcoatRoughness;

// ── Shadow mapping ─────────────────────────────────────
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform bool shadowEnabled;

// ── Fog ────────────────────────────────────────────────
uniform bool  fogEnabled;
uniform vec3  fogColor;
uniform float fogDensity;
uniform int   fogMode;
uniform float fogNear;
uniform float fogFar;

// ── Environment reflection ────────────────────────────────
uniform samplerCube environmentMap;
uniform float environmentIntensity;
uniform bool environmentEnabled;

// ── Point lights ───────────────────────────────────────
struct PointLight {
    vec3 position, ambient, diffuse, specular;
    float constant, linear, quadratic;
};
#define MAX_LIGHTS 8
uniform PointLight pointLights[MAX_LIGHTS];
uniform int numPointLights;

// ============================================================
//  Procedural noise (fallback when textures disabled)
// ============================================================
float hash21(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453);
}
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i+vec2(1,0)), f.x),
               mix(hash21(i+vec2(0,1)), hash21(i+vec2(1,1)), f.x), f.y);
}
float fbm(vec2 p, int octaves) {
    float v=0.0, a=0.5; vec2 s=vec2(0.0);
    for (int i=0; i<8; i++) { if (i>=octaves) break;
        v += a * noise(p+s); s += 1.7; p *= 2.0; a *= 0.5;
    } return v;
}
float voronoi(vec2 p, out float d2) {
    vec2 i=floor(p), f=fract(p); float d1=9.0; d2=9.0;
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++) {
        vec2 n=vec2(dx,dy); vec2 r=vec2(hash21(i+n),hash21(i+n+0.5));
        float d=length(n+r-f);
        if (d<d1) { d2=d1; d1=d; } else if (d<d2) d2=d;
    } return d1;
}

// ── Procedural surface color per planet type ────────────
vec3 proceduralColor(vec2 uv) {
    switch (planetType) {
        case 0: { // Sun
            float d2; float cell=voronoi(uv*30.0,d2);
            float edge=1.0-exp(-(d2-cell)*15.0);
            float gran=fbm(uv*6.0,5)*0.5+0.5;
            float spot=smoothstep(0.65,0.8,fbm(uv*10.0+5.0,3));
            float b=0.55+edge*0.30+gran*0.20-spot*0.35;
            b=clamp(b,0.0,1.0);
            return mix(vec3(1.0,0.55,0.02), vec3(1.0,0.85,0.15), b)*(0.85+edge*0.15);
        }
        case 1: { // Mercury
            float d2; float v=voronoi(uv*18.0,d2);
            float crater=smoothstep(0.05,0.25,d2-v);
            float surf=fbm(uv*7.0,5)*0.4+0.6;
            return vec3(0.50,0.50,0.53)*mix(surf,crater*1.3,0.35);
        }
        case 2: { // Venus
            float w=fbm(uv*2.5+vec2(simTime*0.01,0.0),3)*0.5;
            float band=sin((uv.y+w*0.15)*18.0)*0.3+sin((uv.y+w*0.1)*35.0)*0.2;
            float t=band*0.5+0.5+fbm(uv*4.0,3)*0.3;
            return mix(vec3(0.88,0.75,0.35),vec3(0.95,0.85,0.55),clamp(t,0.0,1.0));
        }
        case 3: { // Earth
            float d2; float v=voronoi(uv*4.5,d2);
            float cell=d2-v; float elev=fbm(uv*5.0,5)*0.5+cell*2.5-0.2;
            float ridge=1.0-abs(fbm(uv*3.0+1.0,4))*1.5;
            elev+=ridge*0.3; float lat=abs(uv.y-0.5)*2.0;
            vec3 col;
            if (elev<-0.05) { float d=(-elev-0.05)*2.5;
                col=mix(vec3(0.02,0.15,0.45),vec3(0.04,0.35,0.65),d); }
            else if (elev<0.10) col=vec3(0.12,0.55,0.30);
            else if (elev<0.22) col=mix(vec3(0.12,0.55,0.30),vec3(0.30,0.45,0.12),(elev-0.10)/0.12);
            else if (elev<0.35) col=mix(vec3(0.30,0.45,0.12),vec3(0.45,0.40,0.28),(elev-0.22)/0.13);
            else if (elev<0.50) col=vec3(0.55,0.50,0.38);
            else col=vec3(0.85,0.82,0.78);
            if (lat>0.82) col=mix(col,vec3(0.92,0.94,0.96),(lat-0.82)/0.18);
            return col;
        }
        case 4: { // Mars
            float d2; float v=voronoi(uv*16.0,d2);
            float crater=smoothstep(0.04,0.20,d2-v);
            float surf=fbm(uv*8.0,5)*0.5+0.5;
            float d=mix(surf,crater*1.4,0.4);
            vec3 base=vec3(0.75,0.28,0.10);
            float lat=abs(uv.y-0.5)*2.0;
            if (lat>0.85) base=mix(base,vec3(0.9,0.85,0.8),(lat-0.85)/0.15);
            return base*d;
        }
        case 5: { // Jupiter
            float warp=fbm(uv*2.0,3)*0.7;
            float band=sin((uv.y+warp*0.15)*28.0)*0.4
                      +sin((uv.y+warp*0.10)*55.0+2.0)*0.25
                      +sin((uv.y+warp*0.06)*100.0+5.0)*0.15+sin(uv.y*220.0)*0.06;
            float turb=fbm(uv*7.0,4)*0.2; float t=band*0.5+0.5+turb;
            float grs=1.0-smoothstep(0.0,0.08,length(uv-vec2(0.55,0.58)));
            grs=grs*grs*0.5;
            vec3 col=mix(vec3(0.72,0.50,0.25),vec3(0.88,0.68,0.38),clamp(t,0.0,1.0));
            return mix(col,vec3(0.85,0.45,0.25),grs);
        }
        case 6: { // Saturn
            float warp=fbm(uv*1.8,3)*0.5;
            float band=sin((uv.y+warp*0.12)*24.0)*0.32+sin((uv.y+warp*0.08)*48.0+1.5)*0.20+sin(uv.y*100.0)*0.08;
            float t=band*0.5+0.5+fbm(uv*5.0,3)*0.12;
            return mix(vec3(0.82,0.72,0.45),vec3(0.95,0.85,0.65),clamp(t,0.0,1.0));
        }
        case 7: { // Uranus
            float band=sin(uv.y*20.0)*0.25+sin(uv.y*40.0+2.0)*0.15;
            float t=band*0.5+0.5+fbm(uv*4.0,3)*0.2;
            return mix(vec3(0.40,0.65,0.78),vec3(0.55,0.80,0.90),clamp(t,0.0,1.0));
        }
        case 8: { // Neptune
            float band=sin(uv.y*18.0)*0.30+sin(uv.y*38.0+1.8)*0.18;
            float t=band*0.5+0.5+fbm(uv*3.5,3)*0.22;
            return mix(vec3(0.12,0.25,0.62),vec3(0.30,0.50,0.82),clamp(t,0.0,1.0));
        }
        default: return vec3(0.5,0.5,0.5);
    }
}

// ============================================================
//  Shadow calculation (PCF — Percentage Closer Filtering)
// ============================================================
float shadowCalculation(vec4 fragPosLightSpace, vec3 N, vec3 L) {
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside light frustum → no shadow
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 1.0;

    float currentDepth = projCoords.z;

    // Bias to avoid shadow acne
    float bias = max(0.0008 * (1.0 - dot(N, L)), 0.00015);

    // PCF: 3x3 kernel
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

// ============================================================
//  GGX Normal Distribution — isotropic variant
// ============================================================
float D_GGX(float NdotH, float alpha) {
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (3.14159 * denom * denom);
}

// ============================================================
//  GGX Normal Distribution — anisotropic variant
//  Uses separate roughness along tangent and bitangent
// ============================================================
float D_GGX_Anisotropic(vec3 N, vec3 H, vec3 T, vec3 B,
                         float roughness, float anisotropy) {
    float aspect = sqrt(1.0 - anisotropy * 0.9);
    float ax = max(roughness * roughness / aspect, 0.0001);
    float ay = max(roughness * roughness * aspect, 0.0001);

    float HdotT = dot(H, T);
    float HdotB = dot(H, B);
    float HdotN = max(dot(H, N), 0.0);

    float expr = (HdotT * HdotT) / (ax * ax) + (HdotB * HdotB) / (ay * ay) + HdotN * HdotN;
    return 1.0 / (3.14159 * ax * ay * expr * expr);
}

// ============================================================
//  Smith GGX Geometry — isotropic
// ============================================================
float G_Smith(float NdotV, float NdotL, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    return G1V * G1L;
}

// ============================================================
//  Schlick Fresnel
// ============================================================
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================
//  Subsurface scattering approximation (wrap + blur)
//  Uses a modified diffuse term that wraps light around edges
// ============================================================
vec3 subsurfaceContribution(vec3 N, vec3 L, vec3 V, vec3 albedo,
                             float sssWeight, vec3 lightColor) {
    if (sssWeight <= 0.0) return vec3(0.0);

    // Wrap lighting: shifts NdotL toward viewer
    float wrap = 0.4;
    float NdotL_wrap = (dot(N, L) + wrap) / (1.0 + wrap);
    NdotL_wrap = max(NdotL_wrap, 0.0);

    // Back-scatter: light transmitting through thin regions
    float backScatter = max(dot(N, -L), 0.0);
    float sss = NdotL_wrap * 0.7 + backScatter * 0.3;

    // Subsurface color is warmer/blurred version of albedo
    vec3 sssColor = mix(albedo, vec3(1.0, 0.7, 0.4), 0.3);
    return sssColor * sss * lightColor * sssWeight * 0.9;
}

// ============================================================
//  Clearcoat layer — extra specular lobe on top
//  Simulates thin transparent coating (ice, water, varnish)
// ============================================================
vec3 clearcoatLayer(vec3 N, vec3 V, vec3 L, vec3 H, float NdotL, float NdotV,
                     float coatStrength, float coatRoughness) {
    if (coatStrength <= 0.0) return vec3(0.0);

    // Fixed IOR for clearcoat (~1.5, polyurethane-like)
    float coatAlpha = max(coatRoughness * coatRoughness, 0.001);
    float NdotH_cc = max(dot(N, H), 0.0);

    float D_c = D_GGX(NdotH_cc, coatAlpha);
    float G_c = G_Smith(NdotV, NdotL, coatRoughness);
    vec3 F_c = F_Schlick(max(dot(V, H), 0.0), vec3(0.04));

    vec3 spec_cc = D_c * G_c * F_c / max(4.0 * NdotL * NdotV, 0.001);
    return spec_cc * NdotL * coatStrength * 1.5;
}

// ============================================================
//  Full PBR Light Calculation
//  Integrates: anisotropic GGX, subsurface, clearcoat, shadows
// ============================================================
vec3 calcPBLight(PointLight light, vec3 N, vec3 V, vec3 albedo,
                 vec3 matAmb, vec3 matDiff, vec3 matSpec,
                 float roughness, float metallic, float anisotropy,
                 float sssWeight, float clearcoat, float clearcoatR,
                 vec3 T, vec3 B, vec4 fragPosLightSpace) {

    vec3 L = normalize(light.position - FragPos);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Attenuation
    float dist = length(light.position - FragPos);
    float att = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    // ── Fresnel ──────────────────────────────────────
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F_Schlick(VdotH, F0);

    // ── Normal Distribution (anisotropic or isotropic) ─
    float D;
    if (anisotropy > 0.001) {
        D = D_GGX_Anisotropic(N, H, T, B, roughness, anisotropy);
    } else {
        float alpha = max(roughness * roughness, 0.001);
        D = D_GGX(NdotH, alpha);
    }

    // ── Geometry ─────────────────────────────────────
    float G = G_Smith(NdotV, NdotL, roughness);

    // ── Specular BRDF ────────────────────────────────
    vec3 specular = (D * G * fresnel) / max(4.0 * NdotL * NdotV, 0.001);

    // ── Diffuse ──────────────────────────────────────
    vec3 diffuse = albedo * (1.0 - metallic) / 3.14159;

    // ── Ambient ──────────────────────────────────────
    vec3 ambient = matAmb * light.ambient * 1.5;

    // ── Directional ──────────────────────────────────
    vec3 directional = (diffuse * matDiff * 0.9 + specular * matSpec) * light.diffuse * NdotL;

    // ── Subsurface scattering ────────────────────────
    vec3 sss = subsurfaceContribution(N, L, V, albedo, sssWeight, light.diffuse);

    // ── Clearcoat ────────────────────────────────────
    vec3 coat = clearcoatLayer(N, V, L, H, NdotL, NdotV, clearcoat, clearcoatR);

    // ── Shadow ───────────────────────────────────────
    float shadow = 1.0;
    if (shadowEnabled && light.constant > 0.9) {  // only shadow from main light
        shadow = shadowCalculation(fragPosLightSpace, N, L);
        shadow = mix(0.15, 1.0, shadow);  // darker shadows for visibility
    }

    return (ambient + (directional + sss + coat) * shadow) * att;
}

// ============================================================
//  Main
// ============================================================
void main()
{
    if (manualCulling && !gl_FrontFacing)
        discard;

    vec2 uv = TexCoord;
    vec3 V = normalize(viewPos - FragPos);

    // ── Normal mapping ─────────────────────────────────
    vec3 N;
    vec3 T = normalize(WorldTangent);
    vec3 B = normalize(WorldBitangent);
    if (useNormalMap) {
        vec3 mappedN = texture(normalMap, uv).rgb * 2.0 - 1.0;
        N = normalize(TBN * mappedN);
    } else {
        N = normalize(Normal);
    }

    // ── Albedo ─────────────────────────────────────────
    vec3 albedo;
    if (useDiffuseMap) {
        albedo = texture(diffuseMap, uv).rgb;
    } else {
        albedo = proceduralColor(uv);
    }

    // ── Specular mask ──────────────────────────────────
    float specMask = 1.0;
    if (useSpecularMap) {
        specMask = texture(specularMap, uv).r;
    }

    // ── Material parameters ────────────────────────────
    vec3  matAmb   = material_ambient;
    vec3  matDiff  = material_diffuse * albedo;
    vec3  matSpec  = material_specular * specMask;
    float roughness = clamp(material_roughness, 0.02, 1.0);
    float metallic  = clamp(material_metallic, 0.0, 1.0);
    float anisotropy = clamp(material_anisotropy, 0.0, 1.0);
    float sssWeight  = clamp(material_subsurface, 0.0, 1.0);
    float clearcoat  = clamp(material_clearcoat, 0.0, 1.0);
    float clearcoatR = clamp(material_clearcoatRoughness, 0.01, 1.0);
    vec3  emissive = material_emissive;

    // Planet-type specific emissive boost for Sun
    if (planetType == 0)
        emissive = albedo * 1.3;

    // ── Compute light-space position for shadow ─────────
    vec4 fragPosLight = lightSpaceMatrix * vec4(FragPos, 1.0);

    // ── Accumulate lighting ────────────────────────────
    vec3 result = emissive + matAmb * 0.25;

    for (int i = 0; i < numPointLights && i < MAX_LIGHTS; i++) {
        result += calcPBLight(pointLights[i], N, V, albedo,
                               matAmb, matDiff, matSpec,
                               roughness, metallic, anisotropy,
                               sssWeight, clearcoat, clearcoatR,
                               T, B, fragPosLight);
    }

    // ── Environment reflection ─────────────────────────
    if (environmentEnabled) {
        vec3 R = reflect(-V, N);
        // Choose mip level based on roughness for blurry reflections
        float mipLevel = roughness * 4.0;
        vec3 envColor = textureLod(environmentMap, R, mipLevel).rgb;

        // Fresnel: more reflection at grazing angles
        vec3 F0_env = mix(vec3(0.04), albedo, metallic);
        vec3 fresnelEnv = F_Schlick(max(dot(N, V), 0.0), F0_env);

        result += envColor * fresnelEnv * environmentIntensity * 1.5;
    }

    // ── Rim light (Fresnel edge glow) ──────────────────
    float rim = 1.0 - abs(dot(N, V));
    result += albedo * pow(rim, 3.0) * 0.25;

    // ── Fog ────────────────────────────────────────────
    if (fogEnabled) {
        float d = length(viewPos - FragPos);
        float ff = 1.0;
        if (fogMode == 0) {
            ff = (fogFar - d) / (fogFar - fogNear);
        } else if (fogMode == 1) {
            ff = exp(-fogDensity * d);
        } else if (fogMode == 2) {
            ff = exp(-fogDensity * d * d);
        } else {
            float heightFactor = exp(-abs(WorldPos.y) * 0.01);
            ff = exp(-fogDensity * d * heightFactor);
        }
        ff = clamp(ff, 0.0, 1.0);

        float horizon = abs(normalize(viewPos - FragPos).y);
        vec3 fogCol = mix(fogColor * 0.5, fogColor * 1.8, smoothstep(0.0, 0.3, horizon));
        result = mix(fogCol, result, ff);
    }

    FragColor = vec4(result, 1.0);
}
