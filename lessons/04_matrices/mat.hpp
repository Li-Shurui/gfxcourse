#pragma once

#include "vec.hpp"

#include <cmath>
#include <optional>

namespace gfx {

struct Mat3 {
  // 列主序。c[0]、c[1]、c[2] 是三个列向量。
  Vec3 c[3]{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

  static Mat3 identity() { return {}; }

  static Mat3 translation(Vec2 offset) {
    Mat3 m;
    m.c[2] = {offset.x, offset.y, 1};
    return m;
  }

  static Mat3 scale(Vec2 s) {
    Mat3 m;
    m.c[0] = {s.x, 0, 0};
    m.c[1] = {0, s.y, 0};
    return m;
  }

  static Mat3 rotation(float radians_value) {
    const float cs = std::cos(radians_value);
    const float sn = std::sin(radians_value);
    Mat3 m;
    m.c[0] = {cs, sn, 0};
    m.c[1] = {-sn, cs, 0};
    return m;
  }

  Vec3 mul(Vec3 v) const { return c[0] * v.x + c[1] * v.y + c[2] * v.z; }
  Vec2 transform_point(Vec2 p) const {
    const Vec3 r = mul({p.x, p.y, 1});
    return {r.x, r.y};
  }
  Vec3 transform_vector(Vec3 v) const { return c[0] * v.x + c[1] * v.y + c[2] * v.z; }
};

inline Mat3 operator*(const Mat3& a, const Mat3& b) {
  Mat3 out;
  out.c[0] = a.mul(b.c[0]);
  out.c[1] = a.mul(b.c[1]);
  out.c[2] = a.mul(b.c[2]);
  return out;
}

inline Mat3 transpose(const Mat3& m) {
  Mat3 out;
  out.c[0] = {m.c[0].x, m.c[1].x, m.c[2].x};
  out.c[1] = {m.c[0].y, m.c[1].y, m.c[2].y};
  out.c[2] = {m.c[0].z, m.c[1].z, m.c[2].z};
  return out;
}

inline std::optional<Mat3> inverse(const Mat3& m) {
  const float det = dot(m.c[0], cross(m.c[1], m.c[2]));
  if (std::fabs(det) < 1e-8f) return std::nullopt;
  const float inv_det = 1.0f / det;
  // 行 r0, r1, r2 构成逆矩阵；再按列主序装回去。
  const Vec3 r0 = cross(m.c[1], m.c[2]) * inv_det;
  const Vec3 r1 = cross(m.c[2], m.c[0]) * inv_det;
  const Vec3 r2 = cross(m.c[0], m.c[1]) * inv_det;
  Mat3 out;
  out.c[0] = {r0.x, r1.x, r2.x};
  out.c[1] = {r0.y, r1.y, r2.y};
  out.c[2] = {r0.z, r1.z, r2.z};
  return out;
}

struct Mat4 {
  // 列主序，下标 col * 4 + row。这和 OpenGL 习惯一致，列向量写在右边：v' = M * v。
  float m[16]{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  float operator()(int row, int col) const { return m[col * 4 + row]; }
  float& operator()(int row, int col) { return m[col * 4 + row]; }

  static Mat4 identity() { return {}; }

  static Mat4 zero() {
    Mat4 out;
    for (float& value : out.m) value = 0;
    return out;
  }

  static Mat4 translation(Vec3 offset) {
    Mat4 out;
    out(0, 3) = offset.x;
    out(1, 3) = offset.y;
    out(2, 3) = offset.z;
    return out;
  }

  static Mat4 scale(Vec3 s) {
    Mat4 out;
    out(0, 0) = s.x;
    out(1, 1) = s.y;
    out(2, 2) = s.z;
    return out;
  }

  static Mat4 rotation(Vec3 axis, float radians_value) {
    const Vec3 u = normalize(axis);
    const float cs = std::cos(radians_value);
    const float sn = std::sin(radians_value);
    const float t = 1.0f - cs;
    Mat4 out{};
    out.m[15] = 1;
    out(0, 0) = t * u.x * u.x + cs;
    out(1, 0) = t * u.x * u.y + sn * u.z;
    out(2, 0) = t * u.x * u.z - sn * u.y;
    out(0, 1) = t * u.x * u.y - sn * u.z;
    out(1, 1) = t * u.y * u.y + cs;
    out(2, 1) = t * u.y * u.z + sn * u.x;
    out(0, 2) = t * u.x * u.z + sn * u.y;
    out(1, 2) = t * u.y * u.z - sn * u.x;
    out(2, 2) = t * u.z * u.z + cs;
    return out;
  }

  Vec3 transform_point(Vec3 p) const;
  Vec3 transform_vector(Vec3 v) const;

  Mat3 upper3() const {
    Mat3 out;
    out.c[0] = {(*this)(0, 0), (*this)(1, 0), (*this)(2, 0)};
    out.c[1] = {(*this)(0, 1), (*this)(1, 1), (*this)(2, 1)};
    out.c[2] = {(*this)(0, 2), (*this)(1, 2), (*this)(2, 2)};
    return out;
  }
};

inline Vec4 operator*(const Mat4& a, Vec4 v) {
  Vec4 out;
  out.x = a(0, 0) * v.x + a(0, 1) * v.y + a(0, 2) * v.z + a(0, 3) * v.w;
  out.y = a(1, 0) * v.x + a(1, 1) * v.y + a(1, 2) * v.z + a(1, 3) * v.w;
  out.z = a(2, 0) * v.x + a(2, 1) * v.y + a(2, 2) * v.z + a(2, 3) * v.w;
  out.w = a(3, 0) * v.x + a(3, 1) * v.y + a(3, 2) * v.z + a(3, 3) * v.w;
  return out;
}

inline Vec3 Mat4::transform_point(Vec3 p) const { return ((*this) * vec4(p, 1)).xyz(); }

inline Vec3 Mat4::transform_vector(Vec3 v) const { return ((*this) * vec4(v, 0)).xyz(); }

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
  Mat4 out{};
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0;
      for (int k = 0; k < 4; ++k) sum += a(row, k) * b(k, col);
      out(row, col) = sum;
    }
  }
  return out;
}

inline std::optional<Mat4> inverse(const Mat4& in) {
  float a[4][8]{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) a[row][col] = in(row, col);
    a[row][row + 4] = 1.0f;
  }
  for (int col = 0; col < 4; ++col) {
    int pivot = col;
    for (int row = col + 1; row < 4; ++row) {
      if (std::fabs(a[row][col]) > std::fabs(a[pivot][col])) pivot = row;
    }
    if (std::fabs(a[pivot][col]) < 1e-8f) return std::nullopt;
    if (pivot != col) {
      for (int k = 0; k < 8; ++k) std::swap(a[col][k], a[pivot][k]);
    }
    const float div = a[col][col];
    for (int k = 0; k < 8; ++k) a[col][k] /= div;
    for (int row = 0; row < 4; ++row) {
      if (row == col) continue;
      const float factor = a[row][col];
      for (int k = 0; k < 8; ++k) a[row][k] -= factor * a[col][k];
    }
  }
  Mat4 out{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) out(row, col) = a[row][col + 4];
  }
  return out;
}

// 法线要乘模型矩阵线性部分的逆转置，非均匀缩放才不会把法线拧歪。
inline Mat3 normal_matrix(const Mat4& model) {
  const Mat3 linear = model.upper3();
  if (const auto inv = inverse(linear)) return transpose(*inv);
  return linear;
}

}  // namespace gfx
