#pragma once

#include "vec.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace gfx {

struct PointLight {
  Vec3 position{0, 2, 2};
  Vec3 color{1, 1, 1};
  float intensity = 1;
};

// Blinn-Phong，计算在线性空间。距离衰减用 intensity / r^2。
inline Vec3 blinn_phong(Vec3 position, Vec3 normal, Vec3 view_position, Vec3 diffuse, Vec3 specular,
                        float shininess, Vec3 ambient, std::span<const PointLight> lights) {
  const float len = length(normal);
  if (len < 1e-8f) return diffuse * ambient;
  const Vec3 n = normal / len;
  const Vec3 view = normalize(view_position - position);
  Vec3 color = diffuse * ambient;
  for (const PointLight& light : lights) {
    const Vec3 delta = light.position - position;
    const float dist2 = std::max(length_squared(delta), 1e-4f);
    const Vec3 l = delta / std::sqrt(dist2);
    const float ndotl = std::max(dot(n, l), 0.0f);
    const Vec3 radiance = light.color * (light.intensity / dist2);
    color += diffuse * radiance * ndotl;
    if (ndotl > 0.0f && shininess > 0.0f) {
      const Vec3 half = normalize(l + view);
      const float spec = std::pow(std::max(dot(n, half), 0.0f), shininess);
      color += specular * radiance * spec;
    }
  }
  return color;
}

}  // namespace gfx
