#include "line.hpp"
#include "log.hpp"
#include "vec.hpp"

#include <cmath>

int main() {
  constexpr int kSize = 360;
  gfx::Image image(kSize, kSize, {14, 16, 22});
  const gfx::Color ink{236, 224, 196};
  const gfx::Color accent{90, 170, 220};

  const int margin = 28;
  gfx::draw_line(image, margin, margin, kSize - margin, margin, accent);
  gfx::draw_line(image, kSize - margin, margin, kSize - margin, kSize - margin, accent);
  gfx::draw_line(image, kSize - margin, kSize - margin, margin, kSize - margin, accent);
  gfx::draw_line(image, margin, kSize - margin, margin, margin, accent);

  // 一束不同斜率的线，用来暴露“只对斜率小于 1 有效”的错误实现。
  const int cx = kSize / 2;
  const int cy = kSize / 2;
  for (int i = 0; i < 16; ++i) {
    const float angle = gfx::radians(static_cast<float>(i) * (360.0f / 16.0f));
    const int x = cx + static_cast<int>(std::cos(angle) * 140.0f);
    const int y = cy + static_cast<int>(std::sin(angle) * 140.0f);
    gfx::draw_line(image, cx, cy, x, y, ink);
  }

  // 小屋：水平、竖直和对角线都要闭合。
  gfx::draw_line(image, 70, 250, 150, 250, {220, 120, 90});
  gfx::draw_line(image, 150, 250, 150, 190, {220, 120, 90});
  gfx::draw_line(image, 150, 190, 70, 190, {220, 120, 90});
  gfx::draw_line(image, 70, 190, 70, 250, {220, 120, 90});
  gfx::draw_line(image, 60, 190, 110, 150, {240, 180, 90});
  gfx::draw_line(image, 110, 150, 160, 190, {240, 180, 90});

  const gfx::Color bg = image.get(0, 0);
  int inked = 0;
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      if (image.get(x, y) != bg) ++inked;
    }
  }
  if (inked < 400) {
    gfx::say_error("直线像素太少");
    return 1;
  }

  const auto path = gfx::output_file("02_lines.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 2 课完成：{}", path.string());
  return 0;
}
