#include "bvh.hpp"
#include "color.hpp"
#include "log.hpp"

#include <chrono>
#include <cstdlib>
#include <vector>

namespace {

bool brute_closest(const std::vector<gfx::Tri>& triangles, const gfx::Ray& ray, float tmin, float tmax, gfx::Hit& hit) {
  bool found = false;
  for (const gfx::Tri& tri : triangles) {
    if (gfx::intersect_triangle(ray, tri, tmin, tmax, hit)) {
      found = true;
      tmax = hit.t;
    }
  }
  return found;
}

gfx::Vec3 shade(const gfx::Hit& hit, const gfx::Ray& ray) {
  const gfx::Vec3 light_pos{2.2f, 3.4f, 2.0f};
  const gfx::Vec3 l = gfx::normalize(light_pos - hit.position);
  const float ndotl = std::max(gfx::dot(hit.normal, l), 0.0f);
  const gfx::Vec3 albedo = hit.material == 0 ? gfx::Vec3{0.62f, 0.64f, 0.6f} : gfx::Vec3{0.25f, 0.48f, 0.86f};
  return albedo * (0.08f + 0.92f * ndotl) + gfx::sky_color(ray) * 0.0f;
}

std::vector<gfx::Tri> make_benchmark_mesh(int slices, int stacks) {
  const gfx::Mesh sphere = gfx::make_uv_sphere(slices, stacks, 1.0f);
  std::vector<gfx::Tri> triangles;
  for (const gfx::Surface& surface : sphere.surfaces) {
    for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3) {
      gfx::Tri tri;
      tri.material = 1;
      for (int k = 0; k < 3; ++k) {
        const gfx::Vertex& vertex = sphere.vertices[surface.indices[i + static_cast<std::size_t>(k)]];
        tri.p[k] = vertex.position + gfx::Vec3{0, 0.2f, 0};
        tri.n[k] = vertex.normal;
        tri.uv[k] = vertex.uv;
      }
      triangles.push_back(tri);
    }
  }
  gfx::Tri a;
  gfx::Tri b;
  const gfx::Vec3 p0{-2, -0.8f, -2};
  const gfx::Vec3 p1{2, -0.8f, -2};
  const gfx::Vec3 p2{2, -0.8f, 2};
  const gfx::Vec3 p3{-2, -0.8f, 2};
  const gfx::Vec3 n{0, 1, 0};
  a.p[0] = p0;
  a.p[1] = p1;
  a.p[2] = p2;
  b.p[0] = p0;
  b.p[1] = p2;
  b.p[2] = p3;
  for (int k = 0; k < 3; ++k) {
    a.n[k] = n;
    b.n[k] = n;
  }
  triangles.push_back(a);
  triangles.push_back(b);
  return triangles;
}

template <class HitFn>
gfx::Image render(int width, int height, const std::vector<gfx::Tri>& triangles, HitFn&& hit_fn) {
  gfx::Image image(width, height, {20, 24, 32});
  gfx::Camera camera;
  camera.eye = {2.4f, 1.6f, 2.8f};
  camera.target = {0, 0.1f, 0};
  camera.fov_y_degrees = 35;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const gfx::Ray ray = gfx::camera_ray(camera, x + 0.5f, y + 0.5f, width, height);
      gfx::Hit hit;
      if (!hit_fn(triangles, ray, 0.001f, 1e30f, hit)) {
        gfx::write_linear(image, x, y, gfx::sky_color(ray));
        continue;
      }
      gfx::write_linear(image, x, y, shade(hit, ray));
    }
  }
  return image;
}

}  // namespace

int main(int argc, char** argv) {
  const bool preview = gfx::has_flag(argc, argv, "--preview");
  const int width = preview ? 80 : 140;
  const int height = preview ? 60 : 100;
  const int slices = preview ? 24 : 36;
  const int stacks = preview ? 12 : 18;
  const std::vector<gfx::Tri> triangles = make_benchmark_mesh(slices, stacks);
  const gfx::Bvh bvh = gfx::Bvh::build(triangles);

  const auto time_of = [](auto&& fn) {
    const auto start = std::chrono::steady_clock::now();
    auto image = fn();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return std::pair{std::move(image), ms};
  };
  const auto brute = time_of([&] {
    return render(width, height, triangles,
                  [](const std::vector<gfx::Tri>& tris, const gfx::Ray& ray, float tmin, float tmax, gfx::Hit& hit) {
                    return brute_closest(tris, ray, tmin, tmax, hit);
                  });
  });
  const auto fast = time_of([&] {
    return render(width, height, triangles,
                  [&](const std::vector<gfx::Tri>& tris, const gfx::Ray& ray, float tmin, float tmax, gfx::Hit& hit) {
                    return bvh.closest(tris, ray, tmin, tmax, hit);
                  });
  });

  int different = 0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const gfx::Color a = brute.first.get(x, y);
      const gfx::Color b = fast.first.get(x, y);
      const int delta = std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b);
      if (delta > 3) ++different;
    }
  }
  if (different > width * height / 100) {
    gfx::say_error("BVH 和暴力求交不一致，差异像素 {}", different);
    return 1;
  }
  if (brute.second > 40.0 && fast.second > brute.second) {
    gfx::say_error("BVH 没有更快：暴力 {:.1f} ms，BVH {:.1f} ms", brute.second, fast.second);
    return 1;
  }

  const auto path = gfx::output_file("14_bvh.bmp");
  if (!fast.first.write_bmp(path)) return 1;
  gfx::say("第 14 课完成：{}（{} 三角形，暴力 {:.1f} ms，BVH {:.1f} ms）", path.string(), triangles.size(), brute.second,
           fast.second);
  return 0;
}
