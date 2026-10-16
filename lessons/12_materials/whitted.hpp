#pragma once

#include "color.hpp"
#include "ray.hpp"
#include "scene.hpp"

#include <algorithm>
#include <cmath>

namespace gfx {

// World 需要提供 closest_hit、occluded、material、shade_albedo、lights、ambient、miss。
template <class World>
Vec3 trace_whitted(const Ray& ray, const World& world, int depth) {
  if (depth <= 0) return {};
  Hit hit;
  if (!world.closest_hit(ray, 1e-3f, 1e30f, hit)) return world.miss(ray);

  const Material& material = world.material(hit.material);
  const Vec3 albedo = world.shade_albedo(material, hit.uv);
  if (material.type == MaterialType::Emissive) return material.emission;

  const Vec3 incident = normalize(ray.direction);
  Vec3 color;
  if (material.type == MaterialType::Diffuse || material.type == MaterialType::Metal) {
    const Vec3 diffuse = material.type == MaterialType::Diffuse ? albedo : albedo * 0.04f;
    const Vec3 specular = material.type == MaterialType::Metal ? albedo : gray(0.04f);
    const float shininess = material.type == MaterialType::Metal
                                ? std::lerp(16.0f, 256.0f, 1.0f - std::clamp(material.roughness, 0.0f, 1.0f))
                                : 32.0f;
    color = diffuse * world.ambient();
    for (const PointLight& light : world.lights()) {
      const Vec3 delta = light.position - hit.position;
      const float dist = length(delta);
      const Vec3 l = delta / dist;
      if (world.occluded(Ray{hit.position + hit.normal * 1e-3f, l}, 1e-3f, dist - 1e-3f)) continue;
      const float ndotl = std::max(dot(hit.normal, l), 0.0f);
      const Vec3 radiance = light.color * (light.intensity / (dist * dist));
      color += diffuse * radiance * ndotl;
      if (ndotl > 0.0f) {
        const Vec3 half = normalize(l - incident);
        color += specular * radiance * std::pow(std::max(dot(hit.normal, half), 0.0f), shininess);
      }
    }
  }

  if (material.type == MaterialType::Metal && depth > 1) {
    const Vec3 reflected = reflect(incident, hit.normal);
    color += albedo * trace_whitted(Ray{hit.position + reflected * 1e-3f, reflected}, world, depth - 1);
  } else if (material.type == MaterialType::Dielectric && depth > 1) {
    const float eta_i = hit.front ? 1.0f : material.ior;
    const float eta_t = hit.front ? material.ior : 1.0f;
    const Vec3 n = hit.front ? hit.geometric : -hit.geometric;
    const float cos_i = std::min(dot(-incident, n), 1.0f);
    Vec3 transmitted;
    float reflect_amount = 1.0f;
    if (refract(incident, n, eta_i / eta_t, transmitted)) {
      reflect_amount = fresnel_dielectric(cos_i, eta_i, eta_t);
    }
    const Vec3 reflected = reflect(incident, n);
    const Vec3 reflect_color =
        trace_whitted(Ray{hit.position + reflected * 1e-3f, reflected}, world, depth - 1);
    if (reflect_amount >= 1.0f) return albedo * reflect_color;
    const Vec3 transmit_color =
        trace_whitted(Ray{hit.position + transmitted * 1e-3f, transmitted}, world, depth - 1);
    return albedo * (reflect_color * reflect_amount + transmit_color * (1.0f - reflect_amount));
  }
  return color;
}

template <class World>
void render_whitted(Image& image, const Camera& camera, const World& world, int depth) {
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const Ray ray = camera_ray(camera, x + 0.5f, y + 0.5f, image.width(), image.height());
      write_linear(image, x, y, trace_whitted(ray, world, depth));
    }
  }
}

}  // namespace gfx
