#pragma once

#include "camera.hpp"

#include <cmath>

namespace gfx {

struct Ray {
  Vec3 origin;
  Vec3 direction;

  Vec3 at(float t) const { return origin + direction * t; }
};

struct Hit {
  float t = 0;
  Vec3 position;
  Vec3 normal;
  Vec3 geometric;
  Vec2 uv;
  int material = -1;
  bool front = true;
};

inline Vec3 sky_color(const Ray& ray) {
  const float t = 0.5f * (normalize(ray.direction).y + 1.0f);
  return lerp(Vec3{1.0f, 1.0f, 1.0f}, Vec3{0.45f, 0.65f, 0.95f}, t);
}

// film_x / film_y 用像素坐标，左上角是 (0, 0)，和帧缓冲一致。
inline Ray camera_ray(const Camera& camera, float film_x, float film_y, int width, int height) {
  const float aspect = static_cast<float>(width) / static_cast<float>(height);
  const float half_height = std::tan(radians(camera.fov_y_degrees) * 0.5f);
  const float half_width = aspect * half_height;
  const Vec3 forward = normalize(camera.target - camera.eye);
  const Vec3 right = normalize(cross(forward, camera.up));
  const Vec3 up = cross(right, forward);
  const float sx = (2.0f * (film_x / static_cast<float>(width)) - 1.0f) * half_width;
  const float sy = (1.0f - 2.0f * (film_y / static_cast<float>(height))) * half_height;
  return {camera.eye, normalize(forward + right * sx + up * sy)};
}

}  // namespace gfx
