#pragma once

#include "image.hpp"

#include <cstdlib>

namespace gfx {

// Bresenham 直线。只使用整数加减，适合把几何边画进像素网格。
inline void draw_line(Image& image, int x0, int y0, int x1, int y1, Color color) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    image.put(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    const int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

}  // namespace gfx
