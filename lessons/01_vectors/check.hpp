#pragma once

#include "log.hpp"
#include "vec.hpp"

#include <cmath>
#include <string_view>

namespace gfx {

inline int& failure_count() {
  static int count = 0;
  return count;
}

inline void check(bool condition, std::string_view expression, std::string_view file, int line) {
  if (!condition) {
    say_error("失败: {}  ({}:{})", expression, file, line);
    ++failure_count();
  }
}

inline bool nearly(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps * std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
}

inline bool nearly(Vec2 a, Vec2 b, float eps = 1e-4f) {
  return nearly(a.x, b.x, eps) && nearly(a.y, b.y, eps);
}

inline bool nearly(Vec3 a, Vec3 b, float eps = 1e-4f) {
  return nearly(a.x, b.x, eps) && nearly(a.y, b.y, eps) && nearly(a.z, b.z, eps);
}

}  // namespace gfx

#define GFX_CHECK(cond) ::gfx::check(static_cast<bool>(cond), #cond, __FILE__, __LINE__)
