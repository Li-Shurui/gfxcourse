#pragma once

#include "ray.hpp"

#include <algorithm>
#include <cmath>

namespace gfx {

struct Sphere {
  Vec3 center;
  float radius = 1;
  int material = 0;

  bool intersect(const Ray& ray, float tmin, float tmax, Hit& hit) const {
    const Vec3 oc = ray.origin - center;
    const float a = length_squared(ray.direction);
    const float half_b = dot(oc, ray.direction);
    const float c = length_squared(oc) - radius * radius;
    const float discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0f) return false;
    const float root = std::sqrt(discriminant);
    float t = (-half_b - root) / a;
    if (t < tmin || t > tmax) {
      t = (-half_b + root) / a;
      if (t < tmin || t > tmax) return false;
    }
    hit.t = t;
    hit.position = ray.at(t);
    const Vec3 outward = (hit.position - center) / radius;
    hit.geometric = outward;
    hit.front = dot(ray.direction, outward) < 0.0f;
    hit.normal = hit.front ? outward : -outward;
    const float phi = std::atan2(-outward.z, outward.x) + kPi;
    const float theta = std::acos(std::clamp(outward.y, -1.0f, 1.0f));
    hit.uv = {phi / (2.0f * kPi), theta / kPi};
    hit.material = material;
    return true;
  }
};

}  // namespace gfx
