#pragma once

#include <algorithm>
#include <cmath>

namespace br {

constexpr float PI = 3.14159265358979323846f;

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 normalize(Vec3 v) {
    float n = std::sqrt(dot(v, v));
    return n > 1e-8f ? v * (1 / n) : Vec3{};
}

// Column-major matrices; shaders multiply projection * view * model * position.
struct Mat4 {
    float m[16]{};

    static Mat4 identity() {
        Mat4 a;
        for (int i = 0; i < 4; i++) {
            a.m[i * 5] = 1;
        }
        return a;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            for (int k = 0; k < 4; k++) {
                r.m[c * 4 + row] += a.m[k * 4 + row] * b.m[c * 4 + k];
            }
        }
    }
    return r;
}

inline Mat4 translate(float x, float y, float z) {
    auto a = Mat4::identity();
    a.m[12] = x;
    a.m[13] = y;
    a.m[14] = z;
    return a;
}

inline Mat4 perspective(float fov, float aspect, float nearPlane, float farPlane) {
    Mat4 a;
    float f = 1 / std::tan(fov * .5f);
    a.m[0] = f / aspect;
    a.m[5] = f;
    a.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    a.m[11] = -1;
    a.m[14] = 2 * farPlane * nearPlane / (nearPlane - farPlane);
    return a;
}

inline Mat4 lookAt(Vec3 eye, Vec3 forward) {
    Vec3 f = normalize(forward);
    Vec3 s = normalize(cross(f, {0, 1, 0}));
    Vec3 u = cross(s, f);
    auto a = Mat4::identity();

    // The camera basis rotates the scene; the last column translates the eye.
    a.m[0] = s.x;
    a.m[4] = s.y;
    a.m[8] = s.z;
    a.m[1] = u.x;
    a.m[5] = u.y;
    a.m[9] = u.z;
    a.m[2] = -f.x;
    a.m[6] = -f.y;
    a.m[10] = -f.z;
    a.m[12] = -dot(s, eye);
    a.m[13] = -dot(u, eye);
    a.m[14] = dot(f, eye);
    return a;
}

inline bool visibleBox(const Mat4& vp, Vec3 center, Vec3 half) {
    // Test the box against all six clip planes. A box is culled only if its
    // entire extent lies outside one plane; partially visible boxes remain.
    for (int axis = 0; axis < 3; axis++) {
        for (int sign : {-1, 1}) {
            float a = vp.m[3] + sign * vp.m[axis];
            float b = vp.m[7] + sign * vp.m[4 + axis];
            float c = vp.m[11] + sign * vp.m[8 + axis];
            float d = vp.m[15] + sign * vp.m[12 + axis];
            if (a * center.x + b * center.y + c * center.z + d
                + std::abs(a) * half.x + std::abs(b) * half.y + std::abs(c) * half.z < 0) {
                return false;
            }
        }
    }
    return true;
}

} // namespace br
