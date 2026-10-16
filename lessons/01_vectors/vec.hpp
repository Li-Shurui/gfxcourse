#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace gfx {

inline constexpr float kPi = std::numbers::pi_v<float>;

constexpr float radians(float degrees) { return degrees * kPi / 180.0f; }
constexpr float degrees(float radians_value) { return radians_value * 180.0f / kPi; }

struct Vec2 {
  float x = 0;
  float y = 0;

  constexpr float operator[](int axis) const { return axis == 0 ? x : y; }
  constexpr float& operator[](int axis) { return axis == 0 ? x : y; }
};

struct Vec3 {
  float x = 0;
  float y = 0;
  float z = 0;

  constexpr float operator[](int axis) const { return axis == 0 ? x : (axis == 1 ? y : z); }
  constexpr float& operator[](int axis) { return axis == 0 ? x : (axis == 1 ? y : z); }
  constexpr Vec2 xy() const { return {x, y}; }
};

struct Vec4 {
  float x = 0;
  float y = 0;
  float z = 0;
  float w = 0;

  constexpr float operator[](int axis) const {
    if (axis == 0) return x;
    if (axis == 1) return y;
    if (axis == 2) return z;
    return w;
  }
  constexpr Vec3 xyz() const { return {x, y, z}; }
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator-(Vec2 a) { return {-a.x, -a.y}; }
constexpr Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }
constexpr Vec2 operator*(float s, Vec2 a) { return a * s; }
constexpr Vec2 operator/(Vec2 a, float s) { return {a.x / s, a.y / s}; }
constexpr Vec2 operator*(Vec2 a, Vec2 b) { return {a.x * b.x, a.y * b.y}; }

constexpr Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
constexpr Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
constexpr Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
constexpr Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
constexpr Vec3 operator*(float s, Vec3 a) { return a * s; }
constexpr Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
constexpr Vec3 operator*(Vec3 a, Vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
constexpr Vec3& operator+=(Vec3& a, Vec3 b) { return a = a + b; }

constexpr Vec4 operator+(Vec4 a, Vec4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
constexpr Vec4 operator-(Vec4 a, Vec4 b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
constexpr Vec4 operator*(Vec4 a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
constexpr Vec4 operator*(float s, Vec4 a) { return a * s; }

constexpr float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
constexpr float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
constexpr float dot(Vec4 a, Vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

constexpr Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

constexpr float length_squared(Vec2 a) { return dot(a, a); }
constexpr float length_squared(Vec3 a) { return dot(a, a); }

inline float length(Vec2 a) { return std::sqrt(length_squared(a)); }
inline float length(Vec3 a) { return std::sqrt(length_squared(a)); }

inline Vec2 normalize(Vec2 a) {
  const float len = length(a);
  return len <= 1e-20f ? Vec2{} : a / len;
}
inline Vec3 normalize(Vec3 a) {
  const float len = length(a);
  return len <= 1e-20f ? Vec3{} : a / len;
}

constexpr Vec3 min(Vec3 a, Vec3 b) {
  return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}
constexpr Vec3 max(Vec3 a, Vec3 b) {
  return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

constexpr Vec3 clamp(Vec3 v, float lo, float hi) {
  return {std::clamp(v.x, lo, hi), std::clamp(v.y, lo, hi), std::clamp(v.z, lo, hi)};
}

constexpr Vec4 vec4(Vec3 v, float w) { return {v.x, v.y, v.z, w}; }
constexpr Vec3 gray(float value) { return {value, value, value}; }

// v 是入射方向（从外侧指向表面），n 是单位法线。
constexpr Vec3 reflect(Vec3 v, Vec3 n) { return v - 2.0f * dot(v, n) * n; }

// eta = ηi / ηt。n 必须朝向入射方向。返回 false 表示全反射。
inline bool refract(Vec3 incident, Vec3 n, float eta, Vec3& transmitted) {
  const float cos_i = std::min(-dot(incident, n), 1.0f);
  const float sin2_t = eta * eta * (1.0f - cos_i * cos_i);
  if (sin2_t > 1.0f) return false;
  const float cos_t = std::sqrt(1.0f - sin2_t);
  transmitted = eta * incident + (eta * cos_i - cos_t) * n;
  return true;
}

inline float fresnel_dielectric(float cos_i, float eta_i, float eta_t) {
  cos_i = std::clamp(cos_i, 0.0f, 1.0f);
  const float sin2_t = (eta_i / eta_t) * (eta_i / eta_t) * (1.0f - cos_i * cos_i);
  if (sin2_t >= 1.0f) return 1.0f;
  const float cos_t = std::sqrt(1.0f - sin2_t);
  const float rs = (eta_i * cos_i - eta_t * cos_t) / (eta_i * cos_i + eta_t * cos_t);
  const float rp = (eta_i * cos_t - eta_t * cos_i) / (eta_i * cos_t + eta_t * cos_i);
  return 0.5f * (rs * rs + rp * rp);
}

inline Vec2 lerp(Vec2 a, Vec2 b, float t) { return a * (1.0f - t) + b * t; }
inline Vec3 lerp(Vec3 a, Vec3 b, float t) { return a * (1.0f - t) + b * t; }

}  // namespace gfx
