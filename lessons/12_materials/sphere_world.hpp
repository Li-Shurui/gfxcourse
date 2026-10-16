#pragma once

#include "scene.hpp"
#include "sphere.hpp"

#include <vector>

namespace gfx {

struct SphereWorld {
  std::vector<Sphere> spheres;
  std::vector<Material> materials;
  std::vector<PointLight> point_lights;
  Vec3 ambient_color = gray(0.02f);

  const Material& material(int index) const { return materials.at(static_cast<std::size_t>(index)); }
  Vec3 shade_albedo(const Material& mat, Vec2) const { return mat.albedo; }
  const std::vector<PointLight>& lights() const { return point_lights; }
  Vec3 ambient() const { return ambient_color; }
  Vec3 miss(const Ray& ray) const { return sky_color(ray); }

  bool closest_hit(const Ray& ray, float tmin, float tmax, Hit& hit) const {
    bool found = false;
    for (const Sphere& sphere : spheres) {
      if (sphere.intersect(ray, tmin, tmax, hit)) {
        found = true;
        tmax = hit.t;
      }
    }
    return found;
  }

  bool occluded(const Ray& ray, float tmin, float tmax) const {
    float start = tmin;
    for (int guard = 0; guard < 8 && start < tmax; ++guard) {
      Hit hit;
      if (!closest_hit(ray, start, tmax, hit)) return false;
      if (material(hit.material).type != MaterialType::Dielectric) return true;
      start = hit.t + 1e-3f;
    }
    return false;
  }
};

}  // namespace gfx
