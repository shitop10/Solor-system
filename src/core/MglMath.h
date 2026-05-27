#pragma once
#include <cmath>
#include <cstring>
#include <cstdio>

// ── Minimal GLM-compatible math library ───────────────
// Replaces GLM dependency. Provides vec2/3/4, mat3/4,
// and common transform functions under the "glm" namespace.

namespace glm {

// ── Constants ─────────────────────────────────────────
template<typename T> T pi() { return T(3.14159265358979323846); }
template<typename T> T radians(T deg) { return deg * pi<T>() / T(180); }

// ── vec2 ──────────────────────────────────────────────
struct vec2 { float x, y;
    vec2() : x(0), y(0) {}
    vec2(float s) : x(s), y(s) {}
    vec2(float _x, float _y) : x(_x), y(_y) {}
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
};
inline vec2 operator+(vec2 a, vec2 b) { return vec2(a.x+b.x, a.y+b.y); }
inline vec2 operator-(vec2 a, vec2 b) { return vec2(a.x-b.x, a.y-b.y); }
inline vec2 operator*(vec2 a, float s) { return vec2(a.x*s, a.y*s); }
inline vec2 operator*(float s, vec2 a) { return vec2(a.x*s, a.y*s); }
inline vec2 operator/(vec2 a, float s) { return vec2(a.x/s, a.y/s); }

// ── vec3 ──────────────────────────────────────────────
struct vec3 { float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
};
inline vec3 operator+(vec3 a, vec3 b) { return vec3(a.x+b.x, a.y+b.y, a.z+b.z); }
inline vec3 operator-(vec3 a, vec3 b) { return vec3(a.x-b.x, a.y-b.y, a.z-b.z); }
inline vec3 operator*(vec3 a, float s) { return vec3(a.x*s, a.y*s, a.z*s); }
inline vec3 operator*(float s, vec3 a) { return vec3(a.x*s, a.y*s, a.z*s); }
inline vec3 operator/(vec3 a, float s) { return vec3(a.x/s, a.y/s, a.z/s); }
inline vec3 operator-(vec3 a) { return vec3(-a.x, -a.y, -a.z); }
inline vec3& operator+=(vec3& a, vec3 b) { a.x+=b.x; a.y+=b.y; a.z+=b.z; return a; }
inline vec3& operator-=(vec3& a, vec3 b) { a.x-=b.x; a.y-=b.y; a.z-=b.z; return a; }
inline vec3& operator*=(vec3& a, float s) { a.x*=s; a.y*=s; a.z*=s; return a; }

inline float dot(vec3 a, vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline vec3 cross(vec3 a, vec3 b) { return vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
inline float length(vec3 v) { return std::sqrt(dot(v, v)); }
inline vec3 normalize(vec3 v) { float len = length(v); return len > 1e-10f ? v / len : vec3(0); }
inline float distance(vec3 a, vec3 b) { return length(a - b); }
inline vec3 mix(vec3 a, vec3 b, float t) { return a + (b - a) * t; }
inline vec3 clamp(vec3 v, float lo, float hi) {
    return vec3(std::fmax(lo, std::fmin(hi, v.x)), std::fmax(lo, std::fmin(hi, v.y)), std::fmax(lo, std::fmin(hi, v.z)));
}

// ── vec4 ──────────────────────────────────────────────
struct vec4 { float x, y, z, w;
    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    vec4(vec3 v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
};
inline vec4 operator+(vec4 a, vec4 b) { return vec4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
inline vec4 operator-(vec4 a, vec4 b) { return vec4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }
inline vec4 operator*(vec4 a, float s) { return vec4(a.x*s, a.y*s, a.z*s, a.w*s); }
inline float dot(vec4 a, vec4 b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

// ── dvec3 (double precision) ──────────────────────────
struct dvec3 { double x, y, z;
    dvec3() : x(0), y(0), z(0) {}
    dvec3(double s) : x(s), y(s), z(s) {}
    dvec3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

// ── mat4 ──────────────────────────────────────────────
struct mat4 {
    float m[4][4];
    mat4() { memset(m, 0, sizeof(m)); }
    mat4(float d) { memset(m, 0, sizeof(m)); m[0][0]=m[1][1]=m[2][2]=m[3][3]=d; }
    float* operator[](int i) { return m[i]; }
    const float* operator[](int i) const { return m[i]; }
};

inline mat4 operator*(const mat4& a, const mat4& b) {
    mat4 r(0);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                r[i][j] += a[i][k] * b[k][j];
    return r;
}

inline vec4 operator*(const mat4& m, const vec4& v) {
    return vec4(
        m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
        m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
        m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
        m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
    );
}

inline vec3 operator*(const mat4& m, const vec3& v) {
    vec4 r = m * vec4(v, 1.0f);
    return vec3(r.x, r.y, r.z);
}

// ── value_ptr (for OpenGL uniform upload) ─────────────
inline const float* value_ptr(const mat4& m) { return &m[0][0]; }
inline const float* value_ptr(const vec3& v) { return &v.x; }
inline const float* value_ptr(const vec4& v) { return &v.x; }

// ── Matrix transforms ─────────────────────────────────
inline mat4 transpose(const mat4& m) {
    mat4 r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[i][j] = m[j][i];
    return r;
}

inline mat4 inverse(const mat4& m) {
    float A2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
    float A1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    float A1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    float A0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
    float A0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
    float A0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
    float det = m[0][0]*(m[1][1]*A2323 - m[1][2]*A1323 + m[1][3]*A1223)
              - m[0][1]*(m[1][0]*A2323 - m[1][2]*A0323 + m[1][3]*A0223)
              + m[0][2]*(m[1][0]*A1323 - m[1][1]*A0323 + m[1][3]*A0123)
              - m[0][3]*(m[1][0]*A1223 - m[1][1]*A0223 + m[1][2]*A0123);
    det = 1.0f / det;
    mat4 r;
    r[0][0] = det * ( m[1][1]*A2323 - m[1][2]*A1323 + m[1][3]*A1223);
    r[0][1] = det * (-m[0][1]*A2323 + m[0][2]*A1323 - m[0][3]*A1223);
    r[0][2] = det * ( m[0][1]*(m[2][2]*m[3][3]-m[2][3]*m[3][2]) - m[0][2]*(m[2][1]*m[3][3]-m[2][3]*m[3][1]) + m[0][3]*(m[2][1]*m[3][2]-m[2][2]*m[3][1]));
    r[0][3] = det * (-m[0][1]*(m[1][2]*m[3][3]-m[1][3]*m[3][2]) + m[0][2]*(m[1][1]*m[3][3]-m[1][3]*m[3][1]) - m[0][3]*(m[1][1]*m[3][2]-m[1][2]*m[3][1]));
    r[1][0] = det * (-m[1][0]*A2323 + m[1][2]*A0323 - m[1][3]*A0223);
    r[1][1] = det * ( m[0][0]*A2323 - m[0][2]*A0323 + m[0][3]*A0223);
    r[1][2] = det * (-m[0][0]*(m[2][2]*m[3][3]-m[2][3]*m[3][2]) + m[0][2]*(m[2][0]*m[3][3]-m[2][3]*m[3][0]) - m[0][3]*(m[2][0]*m[3][2]-m[2][2]*m[3][0]));
    r[1][3] = det * ( m[0][0]*(m[1][2]*m[3][3]-m[1][3]*m[3][2]) - m[0][2]*(m[1][0]*m[3][3]-m[1][3]*m[3][0]) + m[0][3]*(m[1][0]*m[3][2]-m[1][2]*m[3][0]));
    r[2][0] = det * ( m[1][0]*A1323 - m[1][1]*A0323 + m[1][3]*A0123);
    r[2][1] = det * (-m[0][0]*A1323 + m[0][1]*A0323 - m[0][3]*A0123);
    r[2][2] = det * ( m[0][0]*(m[2][1]*m[3][3]-m[2][3]*m[3][1]) - m[0][1]*(m[2][0]*m[3][3]-m[2][3]*m[3][0]) + m[0][3]*(m[2][0]*m[3][1]-m[2][1]*m[3][0]));
    r[2][3] = det * (-m[0][0]*(m[1][1]*m[3][3]-m[1][3]*m[3][1]) + m[0][1]*(m[1][0]*m[3][3]-m[1][3]*m[3][0]) - m[0][3]*(m[1][0]*m[3][1]-m[1][1]*m[3][0]));
    r[3][0] = det * (-m[1][0]*A1223 + m[1][1]*A0223 - m[1][2]*A0123);
    r[3][1] = det * ( m[0][0]*A1223 - m[0][1]*A0223 + m[0][2]*A0123);
    r[3][2] = det * (-m[0][0]*(m[2][1]*m[3][2]-m[2][2]*m[3][1]) + m[0][1]*(m[2][0]*m[3][2]-m[2][2]*m[3][0]) - m[0][2]*(m[2][0]*m[3][1]-m[2][1]*m[3][0]));
    r[3][3] = det * ( m[0][0]*(m[1][1]*m[3][2]-m[1][2]*m[3][1]) - m[0][1]*(m[1][0]*m[3][2]-m[1][2]*m[3][0]) + m[0][2]*(m[1][0]*m[3][1]-m[1][1]*m[3][0]));
    return r;
}

inline mat4 translate(const mat4& m, const vec3& v) {
    mat4 r = m;
    r[0][3] += v.x; r[1][3] += v.y; r[2][3] += v.z;
    return r;
}
inline mat4 scale(const mat4& m, const vec3& v) {
    mat4 r = m;
    r[0][0]*=v.x; r[0][1]*=v.x; r[0][2]*=v.x;
    r[1][0]*=v.y; r[1][1]*=v.y; r[1][2]*=v.y;
    r[2][0]*=v.z; r[2][1]*=v.z; r[2][2]*=v.z;
    return r;
}
inline mat4 rotate(const mat4& m, float angle, const vec3& axis) {
    float c = std::cos(angle), s = std::sin(angle);
    vec3 a = normalize(axis);
    float t = 1.0f - c;
    mat4 rot(1.0f);
    rot[0][0] = c + a.x*a.x*t;       rot[0][1] = a.x*a.y*t - a.z*s; rot[0][2] = a.x*a.z*t + a.y*s;
    rot[1][0] = a.y*a.x*t + a.z*s;   rot[1][1] = c + a.y*a.y*t;       rot[1][2] = a.y*a.z*t - a.x*s;
    rot[2][0] = a.z*a.x*t - a.y*s;   rot[2][1] = a.z*a.y*t + a.x*s;   rot[2][2] = c + a.z*a.z*t;
    return m * rot;
}

// ── Projection functions ──────────────────────────────
inline mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
    mat4 r(1.0f);
    r[0][0] = 2.0f / (right - left);
    r[1][1] = 2.0f / (top - bottom);
    r[2][2] = -2.0f / (far - near);
    r[0][3] = -(right + left) / (right - left);
    r[1][3] = -(top + bottom) / (top - bottom);
    r[2][3] = -(far + near) / (far - near);
    return r;
}

inline mat4 perspective(float fovY, float aspect, float near, float far) {
    float tanHalf = std::tan(fovY * 0.5f);
    mat4 r(0);
    r[0][0] = 1.0f / (aspect * tanHalf);
    r[1][1] = 1.0f / tanHalf;
    r[2][2] = -(far + near) / (far - near);
    r[2][3] = -(2.0f * far * near) / (far - near);
    r[3][2] = -1.0f;
    return r;
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    mat4 r(1.0f);
    r[0][0] = s.x;  r[0][1] = s.y;  r[0][2] = s.z;
    r[1][0] = u.x;  r[1][1] = u.y;  r[1][2] = u.z;
    r[2][0] = -f.x; r[2][1] = -f.y; r[2][2] = -f.z;
    r[0][3] = -dot(s, eye);
    r[1][3] = -dot(u, eye);
    r[2][3] = dot(f, eye);
    return r;
}

// ── Convenience: standalone transform builders ─────────
inline mat4 identity() { return mat4(1.0f); }
inline mat4 translate(vec3 v) {
    mat4 r(1.0f);
    r[0][3] = v.x; r[1][3] = v.y; r[2][3] = v.z;
    return r;
}
inline mat4 scale(vec3 v) {
    mat4 r(1.0f);
    r[0][0] = v.x; r[1][1] = v.y; r[2][2] = v.z;
    return r;
}
inline mat4 rotate(float angle, vec3 axis) {
    return rotate(mat4(1.0f), angle, axis);
}

// ── Clamp ─────────────────────────────────────────────
inline float clamp(float v, float lo, float hi) { return std::fmax(lo, std::fmin(hi, v)); }

} // namespace glm
