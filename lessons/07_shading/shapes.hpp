#pragma once

#include "vec.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace gfx {

struct Geometry {
  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> uvs;
  std::vector<std::uint32_t> indices;
};

inline void add_quad(Geometry& geometry, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal) {
  const std::uint32_t base = static_cast<std::uint32_t>(geometry.positions.size());
  const Vec3 corners[4] = {a, b, c, d};
  const Vec2 uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
  for (int i = 0; i < 4; ++i) {
    geometry.positions.push_back(corners[i]);
    geometry.normals.push_back(normal);
    geometry.uvs.push_back(uvs[i]);
  }
  const std::uint32_t face[6] = {0, 1, 2, 0, 2, 3};
  for (std::uint32_t index : face) geometry.indices.push_back(base + index);
}

inline Geometry make_cube() {
  Geometry geometry;
  add_quad(geometry, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {0, 0, 1});
  add_quad(geometry, {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}, {0, 0, -1});
  add_quad(geometry, {-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, 0, 0});
  add_quad(geometry, {1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, 0, 0});
  add_quad(geometry, {-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {0, 1, 0});
  add_quad(geometry, {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}, {0, -1, 0});
  return geometry;
}

inline Geometry make_plane(float half_extent) {
  Geometry geometry;
  const float h = half_extent;
  add_quad(geometry, {-h, 0, -h}, {h, 0, -h}, {h, 0, h}, {-h, 0, h}, {0, 1, 0});
  return geometry;
}

inline Geometry make_sphere(int slices, int stacks) {
  Geometry geometry;
  for (int y = 0; y <= stacks; ++y) {
    const float v = static_cast<float>(y) / static_cast<float>(stacks);
    const float phi = v * kPi;
    for (int x = 0; x <= slices; ++x) {
      const float u = static_cast<float>(x) / static_cast<float>(slices);
      const float theta = u * kPi * 2.0f;
      const Vec3 normal{std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
      geometry.positions.push_back(normal);
      geometry.normals.push_back(normal);
      geometry.uvs.push_back({u, 1.0f - v});
    }
  }
  const auto index_at = [slices](int x, int y) { return static_cast<std::uint32_t>(y * (slices + 1) + x); };
  for (int y = 0; y < stacks; ++y) {
    for (int x = 0; x < slices; ++x) {
      const std::uint32_t i00 = index_at(x, y);
      const std::uint32_t i10 = index_at(x + 1, y);
      const std::uint32_t i01 = index_at(x, y + 1);
      const std::uint32_t i11 = index_at(x + 1, y + 1);
      geometry.indices.insert(geometry.indices.end(), {i00, i11, i10, i00, i01, i11});
    }
  }
  return geometry;
}

}  // namespace gfx
