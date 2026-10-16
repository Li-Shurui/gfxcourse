#pragma once

#include "line.hpp"

#include <cmath>

namespace gfx {

// Cohen–Sutherland。投影后的坐标可能落在屏幕外很远，不能让 Bresenham 去走几百万步。
inline bool clip_segment_to_rect(float& x0, float& y0, float& x1, float& y1, float xmin, float ymin,
                                 float xmax, float ymax) {
  constexpr int kLeft = 1;
  constexpr int kRight = 2;
  constexpr int kBottom = 4;
  constexpr int kTop = 8;
  const auto code = [&](float x, float y) {
    int bits = 0;
    if (x < xmin) bits |= kLeft;
    else if (x > xmax) bits |= kRight;
    if (y < ymin) bits |= kBottom;
    else if (y > ymax) bits |= kTop;
    return bits;
  };

  int c0 = code(x0, y0);
  int c1 = code(x1, y1);
  for (;;) {
    if ((c0 | c1) == 0) return true;
    if ((c0 & c1) != 0) return false;
    const int out = c0 != 0 ? c0 : c1;
    float x = 0;
    float y = 0;
    if ((out & kTop) != 0) {
      x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
      y = ymax;
    } else if ((out & kBottom) != 0) {
      x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
      y = ymin;
    } else if ((out & kRight) != 0) {
      y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
      x = xmax;
    } else {
      y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
      x = xmin;
    }
    if (out == c0) {
      x0 = x;
      y0 = y;
      c0 = code(x0, y0);
    } else {
      x1 = x;
      y1 = y;
      c1 = code(x1, y1);
    }
  }
}

inline void draw_screen_line(Image& image, float x0, float y0, float x1, float y1, Color color) {
  if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1)) return;
  if (!clip_segment_to_rect(x0, y0, x1, y1, 0.0f, 0.0f, static_cast<float>(image.width() - 1),
                            static_cast<float>(image.height() - 1))) {
    return;
  }
  draw_line(image, static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
            static_cast<int>(std::lround(x1)), static_cast<int>(std::lround(y1)), color);
}

}  // namespace gfx
