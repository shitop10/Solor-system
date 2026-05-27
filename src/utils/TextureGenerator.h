#pragma once
#include <glad/glad.h>
#include "MglMath.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ======================================================
//  Advanced procedural texture engine
//  Techniques: Perlin, Voronoi/cellular, domain warp,
//  ridged multifractal, multi-octave FBM
// ======================================================

static int g_seed = 42;
inline float randf()  { g_seed = g_seed * 1103515245 + 12345; return (float)((g_seed >> 16) & 0x7fff) / 32767.0f; }
inline float randf2(float x, float y) {
    int n = (int)x * 374761393 + (int)y * 668265263;
    n = (n << 13) ^ n;
    return 1.0f - (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

// ── Smoothstep ────────────────────────────────────────
inline float smooth(float t) { return t * t * (3.0f - 2.0f * t); }
inline float smoother(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

// ── Gradient noise (Perlin-style) ─────────────────────
inline float gradNoise(float x, float y) {
    int ix = (int)std::floor(x), iy = (int)std::floor(y);
    float fx = x - ix, fy = y - iy;
    float sx = smoother(fx), sy = smoother(fy);
    float n00 = randf2((float)ix, (float)iy);
    float n10 = randf2((float)ix+1, (float)iy);
    float n01 = randf2((float)ix, (float)iy+1);
    float n11 = randf2((float)ix+1, (float)iy+1);
    float nx0 = n00 + (n10 - n00) * sx;
    float nx1 = n01 + (n11 - n01) * sx;
    return nx0 + (nx1 - nx0) * sy;
}

// ── FBM (Fractal Brownian Motion) ─────────────────────
inline float fbm(float x, float y, int octaves) {
    float val = 0, amp = 1.0f, freq = 1.0f, total = 0;
    for (int i = 0; i < octaves; i++) {
        val += gradNoise(x * freq, y * freq) * amp;
        total += amp;
        amp *= 0.5f; freq *= 2.0f;
    }
    return val / total;
}

// ── Voronoi / Cellular noise ──────────────────────────
inline float voronoi(float x, float y, float& f1, float& f2) {
    int cx = (int)std::floor(x), cy = (int)std::floor(y);
    float fx = x - cx, fy = y - cy;
    f1 = 99.0f; f2 = 99.0f;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float rx = randf2((float)(cx+dx), (float)(cy+dy)) + (float)dx;
            float ry = randf2((float)(cx+dx)+0.5f, (float)(cy+dy)+0.5f) + (float)dy;
            float d = (fx - rx) * (fx - rx) + (fy - ry) * (fy - ry);
            if (d < f1) { f2 = f1; f1 = d; }
            else if (d < f2) { f2 = d; }
        }
    }
    return std::sqrt(f1);
}

// ── Domain warp ───────────────────────────────────────
inline float domainWarp(float x, float y, int octaves) {
    float wx = fbm(x + 3.7f, y + 1.2f, 3) * 0.4f;
    float wy = fbm(x + 8.3f, y + 2.5f, 3) * 0.4f;
    return fbm(x + wx, y + wy, octaves);
}

// ── Ridged multifractal ───────────────────────────────
inline float ridged(float x, float y, int octaves) {
    float val = 0, amp = 1.0f, freq = 1.0f, prev = 1.0f;
    for (int i = 0; i < octaves; i++) {
        float n = std::fabs(gradNoise(x * freq, y * freq));
        n = 1.0f - n; // invert
        n = n * n;    // sharpen ridges
        val += n * amp * prev;
        prev = n;
        amp *= 0.5f; freq *= 2.0f;
    }
    return val;
}

// ------------------------------------------------------
//  Texture Generator
// ------------------------------------------------------
class TextureGenerator {
public:
    // ── Sun: granulation + sunspots ───────────────────
    static unsigned int generateSunTexture(int size) {
        std::vector<unsigned char> data(size * size * 3);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size, v = (float)y / size;
                // Granulation (small-scale cellular)
                float f1, f2;
                voronoi(u * 40.0f, v * 40.0f, f1, f2);
                float cell = f2 - f1; // cell edge distance
                cell = 1.0f - std::exp(-cell * 8.0f);
                // Larger convective cells
                float large = fbm(u * 8, v * 8, 5) * 0.5f + 0.5f;
                // Sunspots (dark regions)
                float spot = std::fabs(fbm(u * 15 + 5.0f, v * 15 + 3.0f, 3)) * 2.0f;
                spot = spot > 0.7f ? (spot - 0.7f) * 3.0f : 0.0f;
                // Combine
                float bright = 0.55f + cell * 0.2f + large * 0.25f - spot * 0.3f;
                bright = std::max(0.1f, std::min(1.0f, bright));
                // Temperature gradient (limb darkening at edges is done in shader)
                int idx = (y * size + x) * 3;
                data[idx]     = (unsigned char)(255 * bright);
                data[idx + 1] = (unsigned char)(160 * bright + large * 60);
                data[idx + 2] = (unsigned char)(10 * bright + spot * 40);
            }
        }
        return createTex(size, size, data.data());
    }

    // ── Earth: Voronoi continents + oceans ────────────
    static unsigned int generateEarthTexture(int size) {
        std::vector<unsigned char> data(size * size * 3);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size, v = (float)y / size;
                // Voronoi for continent shapes
                float f1, f2;
                voronoi(u * 4.5f, v * 4.5f, f1, f2);
                float cellDist = f2 - f1;
                // FBM for height variation on continents
                float height = domainWarp(u * 5, v * 5, 5) * 0.6f + cellDist * 2.5f - 0.2f;
                float mountain = ridged(u * 3, v * 3, 4);
                // Latitude
                float lat = std::fabs(v - 0.5f) * 2.0f; // 0 at equator, 1 at poles
                int idx = (y * size + x) * 3;

                if (height < -0.05f) {
                    // Deep ocean
                    float depth = (-height - 0.05f) / 0.8f;
                    data[idx]=(unsigned char)(10+depth*15); data[idx+1]=(unsigned char)(40+depth*50); data[idx+2]=(unsigned char)(130+depth*90);
                } else if (height < 0.10f) {
                    // Shallow sea / coastal
                    data[idx]=(unsigned char)(30+height*50); data[idx+1]=(unsigned char)(100+height*80); data[idx+2]=(unsigned char)(195+height*30);
                } else if (height < 0.25f) {
                    // Grassland / plains
                    float g = fbm(u*12+5,v*8+5,3)*0.5f+0.5f;
                    // Desert near equator on continents
                    if (lat < 0.3f) { data[idx]=(unsigned char)(180+g*50); data[idx+1]=(unsigned char)(140+g*40); data[idx+2]=(unsigned char)(60+g*20); }
                    else { data[idx]=(unsigned char)(50+g*70); data[idx+1]=(unsigned char)(130+g*60); data[idx+2]=(unsigned char)(30+g*30); }
                } else if (height < 0.40f) {
                    // Forest / hills
                    data[idx]=(unsigned char)(35+height*80); data[idx+1]=(unsigned char)(95+height*60); data[idx+2]=(unsigned char)(20+height*40);
                } else if (height < 0.55f) {
                    // Mountains
                    float m = mountain * 0.6f + 0.4f;
                    data[idx]=(unsigned char)(120+m*50); data[idx+1]=(unsigned char)(115+m*45); data[idx+2]=(unsigned char)(90+m*40);
                } else {
                    // Snow peaks
                    float s = (height - 0.55f) / 0.45f;
                    data[idx]=(unsigned char)(200+(int)(55*s)); data[idx+1]=(unsigned char)(200+(int)(55*s)); data[idx+2]=(unsigned char)(210+(int)(45*s));
                }
                // Polar ice
                if (lat > 0.82f) {
                    float ice = (lat - 0.82f) / 0.18f;
                    ice = std::min(1.0f, ice);
                    data[idx]=(unsigned char)(data[idx]+(245-data[idx])*ice);
                    data[idx+1]=(unsigned char)(data[idx+1]+(248-data[idx+1])*ice);
                    data[idx+2]=(unsigned char)(data[idx+2]+(252-data[idx+2])*ice);
                }
            }
        }
        return createTex(size, size, data.data());
    }

    // ── Jupiter: banded + Great Red Spot ──────────────
    static unsigned int generateJupiterTexture(int size) {
        std::vector<unsigned char> data(size * size * 3);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size, v = (float)y / size;
                // Domain-warped bands
                float warp = fbm(u * 2.5f, v * 2.5f, 3) * 0.8f;
                float band = std::sin((v + warp * 0.15f) * 35.0f) * 0.45f
                           + std::sin((v + warp * 0.1f) * 70.0f + 2.0f) * 0.25f
                           + std::sin((v + warp * 0.05f) * 110.0f + 5.0f) * 0.15f
                           + std::sin(v * 200.0f) * 0.08f
                           + std::sin(v * 350.0f) * 0.04f;
                // Turbulence
                float turb = fbm(u * 8, v * 8, 4) * 0.25f;
                float t = band * 0.5f + 0.5f + turb;
                t = std::max(0.0f, std::min(1.0f, t));
                // Great Red Spot
                float spotDist = std::sqrt((u-0.55f)*(u-0.55f)+(v-0.58f)*(v-0.58f));
                float grs = 0.0f;
                if (spotDist < 0.08f) {
                    grs = (1.0f - spotDist/0.08f) * 0.6f;
                    grs = grs * grs;
                }
                // Colors: brown→tan→cream bands
                int idx = (y * size + x) * 3;
                float r = 0.55f + t * 0.42f + grs * 0.3f;
                float g = 0.30f + t * 0.48f + grs * 0.1f;
                float b = 0.15f + t * 0.35f;
                data[idx]=(unsigned char)(255*std::min(1.0f,r));
                data[idx+1]=(unsigned char)(255*std::min(1.0f,g));
                data[idx+2]=(unsigned char)(255*std::min(1.0f,b));
            }
        }
        return createTex(size, size, data.data());
    }

    // ── Saturn: pale banded ───────────────────────────
    static unsigned int generateSaturnTexture(int size) {
        std::vector<unsigned char> data(size * size * 3);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size, v = (float)y / size;
                float warp = fbm(u * 2, v * 2, 3) * 0.5f;
                float band = std::sin((v+warp*0.12f)*28.0f)*0.35f
                           + std::sin((v+warp*0.08f)*55.0f+1.5f)*0.20f
                           + std::sin(v*120.0f)*0.10f;
                float t = band*0.5f+0.5f+fbm(u*6,v*6,3)*0.15f;
                t = std::max(0.0f,std::min(1.0f,t));
                int idx = (y*size+x)*3;
                data[idx]=(unsigned char)(255*(0.70f+t*0.28f));
                data[idx+1]=(unsigned char)(255*(0.62f+t*0.30f));
                data[idx+2]=(unsigned char)(255*(0.38f+t*0.30f));
            }
        }
        return createTex(size, size, data.data());
    }

    // ── Generic gas giant ─────────────────────────────
    static unsigned int generateGasGiantTexture(int size, glm::vec3 c1, glm::vec3 c2) {
        std::vector<unsigned char> data(size * size * 3);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u=(float)x/size, v=(float)y/size;
                float warp=fbm(u*2,v*2,3)*0.6f;
                float band=std::sin((v+warp*0.13f)*30.0f)*0.40f
                          +std::sin((v+warp*0.09f)*60.0f+2.0f)*0.25f
                          +std::sin(v*100.0f)*0.12f;
                float t=band*0.5f+0.5f+fbm(u*7,v*7,4)*0.18f;
                t=std::max(0.0f,std::min(1.0f,t));
                int idx=(y*size+x)*3;
                data[idx]=(unsigned char)(255*(c1.x+(c2.x-c1.x)*t));
                data[idx+1]=(unsigned char)(255*(c1.y+(c2.y-c1.y)*t));
                data[idx+2]=(unsigned char)(255*(c1.z+(c2.z-c1.z)*t));
            }
        }
        return createTex(size, size, data.data());
    }

    // ── Rocky planet (Mercury/Mars style) ─────────────
    static unsigned int generateRockyPlanetTexture(int size, glm::vec3 base) {
        std::vector<unsigned char> data(size * size * 3);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u=(float)x/size, v=(float)y/size;
                float f1,f2;
                float voro = voronoi(u*14,v*14,f1,f2);
                float crater = 1.0f-std::exp(-(f2-f1)*12.0f); // crater rims
                float surf = domainWarp(u*6,v*6,5)*0.6f+0.4f;
                float detail = surf*0.6f+crater*0.4f;
                int idx=(y*size+x)*3;
                data[idx]=(unsigned char)(255*base.x*detail);
                data[idx+1]=(unsigned char)(255*base.y*detail);
                data[idx+2]=(unsigned char)(255*base.z*detail);
                if (crater>0.55f) { // crater highlight
                    float h=(crater-0.55f)/0.45f;
                    data[idx]=(unsigned char)std::min(255,(int)(data[idx]+40*h));
                    data[idx+1]=(unsigned char)std::min(255,(int)(data[idx+1]+40*h));
                    data[idx+2]=(unsigned char)std::min(255,(int)(data[idx+2]+40*h));
                }
            }
        }
        return createTex(size, size, data.data());
    }

    // ── Normal map ────────────────────────────────────
    static unsigned int generateNormalMap(int size, float strength) {
        std::vector<unsigned char> data(size * size * 3);
        float texel = 1.0f/size;
        for (int y=0;y<size;y++) {
            for (int x=0;x<size;x++) {
                float u=(float)x/size, v=(float)y/size;
                float hL=domainWarp(u-texel,v,4);
                float hR=domainWarp(u+texel,v,4);
                float hD=domainWarp(u,v-texel,4);
                float hU=domainWarp(u,v+texel,4);
                float dx=(hR-hL)*strength;
                float dy=(hU-hD)*strength;
                float len=std::sqrt(dx*dx+dy*dy+1.0f);
                int idx=(y*size+x)*3;
                data[idx]=(unsigned char)(128+127*dx/len);
                data[idx+1]=(unsigned char)(128+127*dy/len);
                data[idx+2]=(unsigned char)(128+127/len);
            }
        }
        return createTex(size,size,data.data());
    }

    // ── Specular map ──────────────────────────────────
    static unsigned int generateSpecularMap(int size, float oceanLevel) {
        std::vector<unsigned char> data(size*size);
        for (int y=0;y<size;y++) {
            for (int x=0;x<size;x++) {
                float u=(float)x/size, v=(float)y/size;
                float f1,f2;
                voronoi(u*4.5f,v*4.5f,f1,f2);
                float h=(f2-f1)*2.5f-0.25f+domainWarp(u*5,v*5,4)*0.5f;
                float spec=h<-0.05f?0.75f:0.08f;
                spec+=fbm(u*3,v*3,3)*0.12f;
                data[y*size+x]=(unsigned char)(255*std::min(1.0f,spec));
            }
        }
        return createTexR(size,size,data.data());
    }

private:
    static unsigned int createTex(int w,int h,const unsigned char* d) {
        unsigned int t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,d);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        return t;
    }
    static unsigned int createTexR(int w,int h,const unsigned char* d) {
        unsigned int t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RED,w,h,0,GL_RED,GL_UNSIGNED_BYTE,d);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        return t;
    }
};
