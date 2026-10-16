#pragma once

#include "ray.hpp"

#include <cmath>

namespace gfx {

struct Tri {
  Vec3 p[3];
  Vec3 n[3];
  Vec2 uv[3];
  int material = 0;
};

// Möller–Trumbore。u、v 是顶点 1 和顶点 2 的重心坐标。
inline bool intersect_triangle(const Ray& ray, const Tri& tri, float tmin, float tmax, Hit& hit) {
  constexpr float kEps = 1e-8f;
  const Vec3 e1 = tri.p[1] - tri.p[0];
  const Vec3 e2 = tri.p[2] - tri.p[0];
  const Vec3 pvec = cross(ray.direction, e2);
  const float det = dot(e1, pvec);
  if (std::fabs(det) < kEps) return false;
  const float inv_det = 1.0f / det;
  const Vec3 tvec = ray.origin - tri.p[0];
  const float u = dot(tvec, pvec) * inv_det;
  if (u < 0.0f || u > 1.0f) return false;
  const Vec3 qvec = cross(tvec, e1);
  const float v = dot(ray.direction, qvec) * inv_det;
  if (v < 0.0f || u + v > 1.0f) return false;
  const float t = dot(e2, qvec) * inv_det;
  if (t < tmin || t > tmax) return false;

  const float w = 1.0f - u - v;
  Vec3 outward = cross(e1, e2);
  if (length_squared(outward) < kEps) return false;
  outward = normalize(outward);
  Vec3 shading = tri.n[0] * w + tri.n[1] * u + tri.n[2] * v;
  if (length_squared(shading) < kEps) shading = outward;
  else shading = normalize(shading);

  hit.t = t;
  hit.position = ray.at(t);
  hit.geometric = outward;
  hit.front = dot(ray.direction, outward) < 0.0f;
  const Vec3 face = hit.front ? outward : -outward;
  if (dot(shading, face) < 0.0f) shading = -shading;
  hit.normal = shading;
  hit.uv = tri.uv[0] * w + tri.uv[1] * u + tri.uv[2] * v;
  hit.material = tri.material;
  return true;
}

}  // namespace gfx
