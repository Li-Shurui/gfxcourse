#include "check.hpp"
#include "image.hpp"
#include "log.hpp"
#include "vec.hpp"

int main() {
  using gfx::Vec3;
  const Vec3 i{1, 0, 0};
  const Vec3 j{0, 1, 0};
  const Vec3 k{0, 0, 1};

  GFX_CHECK(gfx::nearly(gfx::cross(i, j), k));
  GFX_CHECK(gfx::nearly(gfx::cross(j, k), i));
  GFX_CHECK(gfx::nearly(gfx::dot(i, j), 0.0f));
  GFX_CHECK(gfx::nearly(gfx::normalize(Vec3{3, 0, 0}), i));
  GFX_CHECK(gfx::nearly(gfx::reflect(Vec3{1, -1, 0}, j), Vec3{1, 1, 0}));

  Vec3 bent;
  const bool transmitted = gfx::refract(gfx::normalize(Vec3{0.2f, -1, 0}), j, 1.0f / 1.5f, bent);
  GFX_CHECK(transmitted);
  GFX_CHECK(bent.y < 0.0f);

  const float normal_incidence = gfx::fresnel_dielectric(1.0f, 1.0f, 1.5f);
  GFX_CHECK(normal_incidence > 0.03f && normal_incidence < 0.05f);

  // 把向量画出来：x 红、y 绿、z 在这个二维图里用蓝色圆点表示它的 z 分量。
  constexpr int kSize = 280;
  gfx::Image image(kSize, kSize, {16, 18, 24});
  const gfx::Vec2 origin{40.0f, 230.0f};
  const auto plot = [&](gfx::Vec2 tip, gfx::Color color) {
    const gfx::Vec2 head = origin + tip;
    // 这一课还没有直线算法，先用采样把箭头涂出来。
    const int steps = static_cast<int>(gfx::length(tip));
    for (int s = 0; s <= steps; ++s) {
      const float t = steps == 0 ? 0.0f : static_cast<float>(s) / static_cast<float>(steps);
      const gfx::Vec2 p = gfx::lerp(origin, head, t);
      image.fill_rect(static_cast<int>(p.x) - 1, static_cast<int>(p.y) - 1, 3, 3, color);
    }
  };
  plot({180, 0}, {220, 70, 70});
  plot({0, -180}, {70, 200, 110});
  plot(gfx::normalize(gfx::Vec2{1, -1}) * 150.0f, {240, 210, 90});

  const auto path = gfx::output_file("01_vectors.bmp");
  if (!image.write_bmp(path)) {
    gfx::say_error("写出图像失败");
    return 1;
  }
  if (gfx::failure_count() != 0) return 1;
  gfx::say("第 1 课完成，向量自检通过：{}", path.string());
  return 0;
}
