#include "log.hpp"
#include "triangle.hpp"

#include <vector>

namespace {

int coverage_self_check() {
  constexpr int kSize = 4;
  std::vector<int> owner(kSize * kSize, 0);
  const gfx::Vec2 tris[2][3] = {
      {{0, 0}, {4, 0}, {4, 4}},
      {{0, 0}, {4, 4}, {0, 4}},
  };
  for (int t = 0; t < 2; ++t) {
    for (int y = 0; y < kSize; ++y) {
      for (int x = 0; x < kSize; ++x) {
        const gfx::Vec2 p{x + 0.5f, y + 0.5f};
        if (gfx::barycentric(p, tris[t][0], tris[t][1], tris[t][2]).inside) {
          owner[static_cast<std::size_t>(y * kSize + x)] += (t == 0 ? 1 : 2);
        }
      }
    }
  }
  for (int value : owner) {
    if (value != 1 && value != 2) return 1;
  }
  return 0;
}

}  // namespace

int main() {
  if (coverage_self_check() != 0) {
    gfx::say_error("三角形填充规则有裂缝或重叠");
    return 1;
  }

  gfx::Image image(420, 320, {17, 19, 27});
  gfx::fill_triangle(image, {40, 40}, {250, 70}, {80, 250}, {232, 96, 80});
  gfx::fill_triangle(image, {180, 40}, {390, 150}, {210, 290}, {232, 96, 80}, {246, 214, 92},
                     {86, 168, 214});

  // 两个三角形拼成矩形，对角线不能裂开。
  gfx::fill_triangle(image, {250, 200}, {390, 200}, {390, 290}, {120, 90, 180});
  gfx::fill_triangle(image, {250, 200}, {390, 290}, {250, 290}, {70, 140, 160});

  const auto path = gfx::output_file("03_triangles.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 3 课完成：{}", path.string());
  return 0;
}
