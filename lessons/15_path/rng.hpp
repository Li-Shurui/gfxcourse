#pragma once

#include "vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gfx {

// SplitMix64。路径追踪要大量随机数，这个生成器比 std::mt19937 更短，也够用。
struct Rng {
  std::uint64_t state;

  explicit Rng(std::uint64_t seed) : state(seed == 0 ? 0x9e3779b97f4a7c15ull : seed) {}

  std::uint32_t next_u32() {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return static_cast<std::uint32_t>(z ^ (z >> 31));
  }

  float uniform() { return static_cast<float>(next_u32() >> 8) * (1.0f / 16777216.0f); }

  Vec3 in_unit_sphere() {
    for (;;) {
      const Vec3 point{uniform() * 2.0f - 1.0f, uniform() * 2.0f - 1.0f, uniform() * 2.0f - 1.0f};
      if (length_squared(point) < 1.0f) return point;
    }
  }
};

inline Vec3 sample_cosine_hemisphere(Vec3 normal, Rng& rng) {
  const float u1 = rng.uniform();
  const float u2 = rng.uniform();
  const float radius = std::sqrt(u1);
  const float phi = 2.0f * kPi * u2;
  const Vec3 local{radius * std::cos(phi), std::sqrt(std::max(0.0f, 1.0f - u1)), radius * std::sin(phi)};
  const Vec3 helper = std::fabs(normal.y) < 0.999f ? Vec3{0, 1, 0} : Vec3{1, 0, 0};
  const Vec3 tangent = normalize(cross(helper, normal));
  const Vec3 bitangent = cross(normal, tangent);
  return tangent * local.x + normal * local.y + bitangent * local.z;
}

}  // namespace gfx
