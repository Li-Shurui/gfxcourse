#pragma once

#include "bvh.hpp"
#include "parallel.hpp"
#include "rng.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace gfx {

inline bool scene_occluded(const RWorld& world, const Bvh& bvh, const Ray& ray, float tmin, float tmax) {
  float start = tmin;
  for (int guard = 0; guard < 8 && start < tmax; ++guard) {
    Hit hit;
    if (!hit_scene(world, bvh, ray, start, tmax, hit)) return false;
    if (world.material(hit.material).type != MaterialType::Dielectric) return true;
    start = hit.t + 1e-3f;
  }
  return false;
}

inline Vec3 sample_direct(const RWorld& world, const Bvh& bvh, const Hit& hit, Vec3 albedo, Rng& rng) {
  const std::vector<AreaLight>& lights = world.scene->area_lights;
  if (lights.empty()) return {};
  const int count = static_cast<int>(lights.size());
  const int index = std::min(count - 1, static_cast<int>(rng.uniform() * static_cast<float>(count)));
  const AreaLight& light = lights[static_cast<std::size_t>(index)];
  const Vec3 point = light.sample(rng.uniform(), rng.uniform());
  const Vec3 delta = point - hit.position;
  const float dist2 = length_squared(delta);
  const float dist = std::sqrt(dist2);
  if (dist < 1e-4f) return {};
  const Vec3 wi = delta / dist;
  const float cos_surface = dot(hit.normal, wi);
  const float cos_light = dot(light.normal(), -wi);
  if (cos_surface <= 0.0f || cos_light <= 0.0f) return {};
  if (scene_occluded(world, bvh, Ray{hit.position + hit.normal * 1e-3f, wi}, 1e-3f, dist - 2e-3f)) return {};
  const Vec3 brdf = albedo * (1.0f / kPi);
  return brdf * light.emission * cos_surface * cos_light * (static_cast<float>(count) * light.area()) / dist2;
}

inline Vec3 trace_path(const RWorld& world, const Bvh& bvh, Ray ray, Rng& rng) {
  Vec3 radiance;
  Vec3 throughput{1, 1, 1};
  bool count_emission = true;
  constexpr int kMaxDepth = 6;
  for (int bounce = 0; bounce < kMaxDepth; ++bounce) {
    Hit hit;
    if (!hit_scene(world, bvh, ray, 1e-3f, 1e30f, hit)) {
      radiance += throughput * world.miss(ray);
      break;
    }
    const Material& material = world.material(hit.material);
    const Vec3 albedo = world.shade_albedo(material, hit.uv);
    if (material.type == MaterialType::Emissive) {
      if (count_emission) radiance += throughput * material.emission;
      break;
    }

    if (material.type == MaterialType::Diffuse) {
      radiance += throughput * sample_direct(world, bvh, hit, albedo, rng);
      const Vec3 wi = sample_cosine_hemisphere(hit.normal, rng);
      if (length_squared(wi) < 1e-8f) break;
      throughput = throughput * albedo;
      count_emission = false;
      ray = {hit.position + hit.normal * 1e-3f, normalize(wi)};
    } else if (material.type == MaterialType::Metal) {
      Vec3 reflected = reflect(normalize(ray.direction), hit.normal);
      if (material.roughness > 0.0f) {
        reflected = normalize(reflected + material.roughness * rng.in_unit_sphere());
        if (dot(reflected, hit.normal) <= 0.0f) break;
      }
      throughput = throughput * albedo;
      count_emission = true;
      ray = {hit.position + reflected * 1e-3f, reflected};
    } else {
      const Vec3 incident = normalize(ray.direction);
      const float eta_i = hit.front ? 1.0f : material.ior;
      const float eta_t = hit.front ? material.ior : 1.0f;
      const Vec3 n = hit.front ? hit.geometric : -hit.geometric;
      const float cos_i = std::min(dot(-incident, n), 1.0f);
      Vec3 transmitted;
      const float reflect_amount =
          refract(incident, n, eta_i / eta_t, transmitted) ? fresnel_dielectric(cos_i, eta_i, eta_t) : 1.0f;
      if (rng.uniform() < reflect_amount) {
        const Vec3 reflected = reflect(incident, n);
        ray = {hit.position + reflected * 1e-3f, reflected};
      } else {
        throughput = throughput * albedo;
        ray = {hit.position + transmitted * 1e-3f, transmitted};
      }
      count_emission = true;
    }

    if (bounce >= 2) {
      const float survive = std::clamp(std::max(throughput.x, std::max(throughput.y, throughput.z)), 0.05f, 0.95f);
      if (rng.uniform() > survive) break;
      throughput = throughput * (1.0f / survive);
    }
  }
  if (!std::isfinite(radiance.x) || !std::isfinite(radiance.y) || !std::isfinite(radiance.z)) return {};
  return min(radiance, gray(16.0f));
}

inline Vec3 tone_map(Vec3 color) {
  const Vec3 exposed = color * 0.85f;
  return {exposed.x / (1.0f + exposed.x), exposed.y / (1.0f + exposed.y), exposed.z / (1.0f + exposed.z)};
}

inline void render_path(const RWorld& world, const Bvh& bvh, const Camera& camera, Image& image, int samples) {
  const int width = image.width();
  const int height = image.height();
  std::vector<Vec3> film(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
  parallel_rows(height, [&](int y0, int y1) {
    for (int y = y0; y < y1; ++y) {
      for (int x = 0; x < width; ++x) {
        Rng rng((static_cast<std::uint64_t>(y) << 32) ^ static_cast<std::uint64_t>(x) * 9781ull ^ 0x123456789ull);
        Vec3 sum;
        for (int sample = 0; sample < samples; ++sample) {
          const Ray ray = camera_ray(camera, static_cast<float>(x) + rng.uniform(),
                                     static_cast<float>(y) + rng.uniform(), width, height);
          sum += trace_path(world, bvh, ray, rng);
        }
        film[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] =
            sum * (1.0f / static_cast<float>(samples));
      }
    }
  });
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const Vec3 color = film[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)];
      write_linear(image, x, y, tone_map(color));
    }
  }
}

}  // namespace gfx
