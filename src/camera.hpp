#pragma once

#include "mat.hpp"

#include <cmath>

namespace gfx {

struct Camera {
  Vec3 eye{0, 0, 3};
  Vec3 target{0, 0, 0};
  Vec3 up{0, 1, 0};
  float fov_y_degrees = 45;
  float z_near = 0.1f;
  float z_far = 100;
};

// 右手系，y 向上，相机看向 -Z。
inline Mat4 view_matrix(const Camera& camera) {
  const Vec3 forward = normalize(camera.target - camera.eye);
  const Vec3 right = normalize(cross(forward, camera.up));
  const Vec3 up = cross(right, forward);
  Mat4 m = Mat4::identity();
  m(0, 0) = right.x;
  m(0, 1) = right.y;
  m(0, 2) = right.z;
  m(0, 3) = -dot(right, camera.eye);
  m(1, 0) = up.x;
  m(1, 1) = up.y;
  m(1, 2) = up.z;
  m(1, 3) = -dot(up, camera.eye);
  m(2, 0) = -forward.x;
  m(2, 1) = -forward.y;
  m(2, 2) = -forward.z;
  m(2, 3) = dot(forward, camera.eye);
  return m;
}

// OpenGL 风格透视：近平面映射到 NDC z = -1，远平面映射到 +1。必须从零矩阵开始，
// 因为 m(3,3) 是 0，不是单位矩阵里的 1。
inline Mat4 perspective(float fov_y_degrees, float aspect, float z_near, float z_far) {
  const float f = 1.0f / std::tan(radians(fov_y_degrees) * 0.5f);
  Mat4 m = Mat4::zero();
  m(0, 0) = f / aspect;
  m(1, 1) = f;
  m(2, 2) = (z_far + z_near) / (z_near - z_far);
  m(3, 2) = -1.0f;
  m(2, 3) = (2.0f * z_far * z_near) / (z_near - z_far);
  return m;
}

inline Mat4 view_projection(const Camera& camera, float aspect) {
  return perspective(camera.fov_y_degrees, aspect, camera.z_near, camera.z_far) * view_matrix(camera);
}

// 近平面 z + w = 0。线段有一个端点在相机后面时，把该端点拉到平面上。
inline bool clip_line_near(Vec4& a, Vec4& b) {
  const float da = a.z + a.w;
  const float db = b.z + b.w;
  if (da < 0.0f && db < 0.0f) return false;
  if (da >= 0.0f && db >= 0.0f) return true;
  const float denom = da - db;
  if (std::fabs(denom) < 1e-8f) return false;
  const Vec4 point = a + (b - a) * (da / denom);
  if (da < 0.0f) a = point;
  else b = point;
  return true;
}

inline bool project_to_screen(Vec4 clip, int width, int height, float& x, float& y) {
  if (!(clip.w > 1e-6f) || !std::isfinite(clip.w)) return false;
  const float inv_w = 1.0f / clip.w;
  const float ndc_x = clip.x * inv_w;
  const float ndc_y = clip.y * inv_w;
  x = (ndc_x + 1.0f) * 0.5f * static_cast<float>(width);
  y = (1.0f - (ndc_y + 1.0f) * 0.5f) * static_cast<float>(height);
  return std::isfinite(x) && std::isfinite(y);
}

}  // namespace gfx
