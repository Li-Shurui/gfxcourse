#pragma once

#include "image.hpp"
#include "vec.hpp"

#include <algorithm>
#include <cmath>

namespace gfx {

// 有向边函数。屏幕 y 向下时，三点呈正面积的绕序与数学课本里的逆时针相反，以自检为准。
inline float edge(Vec2 a, Vec2 b, Vec2 p) {
  return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

// 上边或左边。共享边上的像素只归一边，避免两条三角形之间出现裂缝或双写。
inline bool is_top_left(Vec2 start, Vec2 end) {
  return (start.y == end.y && start.x > end.x) || (end.y > start.y);
}

struct Barycentric {
  float w0 = 0;
  float w1 = 0;
  float w2 = 0;
  bool inside = false;
};

// 权重对应原始顶点 a、b、c。函数内部可能交换绕序，但返回的权重仍按传入顺序。
inline Barycentric barycentric(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
  const float signed_area = edge(a, b, c);
  if (signed_area == 0.0f) return {};
  const float inv_area = 1.0f / signed_area;
  Barycentric out;
  out.w0 = edge(b, c, p) * inv_area;
  out.w1 = edge(c, a, p) * inv_area;
  out.w2 = edge(a, b, p) * inv_area;

  Vec2 s0 = a;
  Vec2 s1 = b;
  Vec2 s2 = c;
  if (signed_area < 0.0f) {
    std::swap(s1, s2);
  }
  float e0 = edge(s1, s2, p);
  float e1 = edge(s2, s0, p);
  float e2 = edge(s0, s1, p);
  const auto accept = [](float w, bool top_left) { return w > 0.0f || (w == 0.0f && top_left); };
  out.inside = accept(e0, is_top_left(s1, s2)) && accept(e1, is_top_left(s2, s0)) &&
               accept(e2, is_top_left(s0, s1));
  return out;
}

inline void fill_triangle(Image& image, Vec2 a, Vec2 b, Vec2 c, Color color) {
  if (!std::isfinite(a.x) || !std::isfinite(b.x) || !std::isfinite(c.x)) return;
  const float min_x = std::min({a.x, b.x, c.x});
  const float max_x = std::max({a.x, b.x, c.x});
  const float min_y = std::min({a.y, b.y, c.y});
  const float max_y = std::max({a.y, b.y, c.y});
  const int x0 = std::clamp(static_cast<int>(std::floor(min_x)), 0, image.width() - 1);
  const int x1 = std::clamp(static_cast<int>(std::floor(max_x)), 0, image.width() - 1);
  const int y0 = std::clamp(static_cast<int>(std::floor(min_y)), 0, image.height() - 1);
  const int y1 = std::clamp(static_cast<int>(std::floor(max_y)), 0, image.height() - 1);

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const Vec2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
      if (barycentric(p, a, b, c).inside) image.put(x, y, color);
    }
  }
}

inline void fill_triangle(Image& image, Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc) {
  if (!std::isfinite(a.x) || !std::isfinite(b.x) || !std::isfinite(c.x)) return;
  const float min_x = std::min({a.x, b.x, c.x});
  const float max_x = std::max({a.x, b.x, c.x});
  const float min_y = std::min({a.y, b.y, c.y});
  const float max_y = std::max({a.y, b.y, c.y});
  const int x0 = std::clamp(static_cast<int>(std::floor(min_x)), 0, std::max(image.width() - 1, 0));
  const int x1 = std::clamp(static_cast<int>(std::floor(max_x)), 0, std::max(image.width() - 1, 0));
  const int y0 = std::clamp(static_cast<int>(std::floor(min_y)), 0, std::max(image.height() - 1, 0));
  const int y1 = std::clamp(static_cast<int>(std::floor(max_y)), 0, std::max(image.height() - 1, 0));
  if (image.empty()) return;

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const Vec2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
      const Barycentric bary = barycentric(p, a, b, c);
      if (!bary.inside) continue;
      const auto mix = [&](std::uint8_t c0, std::uint8_t c1, std::uint8_t c2) {
        const float value = bary.w0 * static_cast<float>(c0) + bary.w1 * static_cast<float>(c1) +
                            bary.w2 * static_cast<float>(c2);
        return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 255.0f));
      };
      image.put(x, y, {mix(ca.r, cb.r, cc.r), mix(ca.g, cb.g, cc.g), mix(ca.b, cb.b, cc.b)});
    }
  }
}

}  // namespace gfx
