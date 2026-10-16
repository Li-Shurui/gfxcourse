#pragma once

#include "color.hpp"

#include <algorithm>
#include <cmath>

namespace gfx {

struct Texture {
  enum class Filter { Nearest, Bilinear };

  Image image;
  Filter filter = Filter::Bilinear;

  Vec3 fetch(int x, int y) const {
    const int w = image.width();
    const int h = image.height();
    if (w <= 0 || h <= 0) return {};
    auto wrap = [](int i, int n) {
      const int m = i % n;
      return m < 0 ? m + n : m;
    };
    return srgb_to_linear(image.get(wrap(x, w), wrap(y, h)));
  }

  // u 向右、v 向上，和 OBJ 的纹理坐标一致。采样时会把 v 翻到图像的 y 向下。
  Vec3 sample(Vec2 uv) const {
    if (image.empty()) return gray(1);
    const float u = uv.x - std::floor(uv.x);
    const float v = 1.0f - (uv.y - std::floor(uv.y));
    const float width = static_cast<float>(image.width());
    const float height = static_cast<float>(image.height());
    if (filter == Filter::Nearest) {
      const int x = std::clamp(static_cast<int>(u * width), 0, image.width() - 1);
      const int y = std::clamp(static_cast<int>(v * height), 0, image.height() - 1);
      return srgb_to_linear(image.get(x, y));
    }

    const float sx = u * width - 0.5f;
    const float sy = v * height - 0.5f;
    const int x0 = static_cast<int>(std::floor(sx));
    const int y0 = static_cast<int>(std::floor(sy));
    const float tx = sx - static_cast<float>(x0);
    const float ty = sy - static_cast<float>(y0);
    const Vec3 c00 = fetch(x0, y0);
    const Vec3 c10 = fetch(x0 + 1, y0);
    const Vec3 c01 = fetch(x0, y0 + 1);
    const Vec3 c11 = fetch(x0 + 1, y0 + 1);
    const Vec3 top = lerp(c00, c10, tx);
    const Vec3 bottom = lerp(c01, c11, tx);
    return lerp(top, bottom, ty);
  }
};

inline Texture make_checker(int cells, int cell_size, Color a, Color b) {
  const int n = cells * cell_size;
  Texture texture;
  texture.image = Image(n, n);
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      const bool on = ((x / cell_size) + (y / cell_size)) % 2 == 0;
      texture.image.put(x, y, on ? a : b);
    }
  }
  return texture;
}

}  // namespace gfx
