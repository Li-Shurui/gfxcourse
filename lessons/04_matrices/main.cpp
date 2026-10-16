#include "check.hpp"
#include "line.hpp"
#include "log.hpp"
#include "mat.hpp"

#include <cmath>
#include <vector>

namespace {

bool mat4_close(const gfx::Mat4& a, const gfx::Mat4& b) {
  for (int i = 0; i < 16; ++i) {
    if (!gfx::nearly(a.m[i], b.m[i], 1e-4f)) return false;
  }
  return true;
}

void draw_polygon(gfx::Image& image, const std::vector<gfx::Vec2>& points, gfx::Color color) {
  for (std::size_t i = 0; i < points.size(); ++i) {
    const gfx::Vec2 a = points[i];
    const gfx::Vec2 b = points[(i + 1) % points.size()];
    gfx::draw_line(image, static_cast<int>(std::lround(a.x)), static_cast<int>(std::lround(a.y)),
                   static_cast<int>(std::lround(b.x)), static_cast<int>(std::lround(b.y)), color);
  }
}

}  // namespace

int main() {
  using gfx::Mat4;
  using gfx::Vec3;

  const Vec3 x_axis = Mat4::rotation({0, 1, 0}, gfx::radians(90.0f)).transform_point({1, 0, 0});
  GFX_CHECK(gfx::nearly(x_axis, Vec3{0, 0, -1}));
  const Vec3 moved = Mat4::translation({2, 3, 4}).transform_point({1, 1, 1});
  GFX_CHECK(gfx::nearly(moved, Vec3{3, 4, 5}));

  const Mat4 trs = Mat4::translation({1, 2, 3}) * Mat4::rotation({0, 1, 0}, gfx::radians(25.0f)) *
                   Mat4::scale({1.5f, 0.5f, 2.0f});
  const auto inv = gfx::inverse(trs);
  GFX_CHECK(inv.has_value());
  if (inv) GFX_CHECK(mat4_close(trs * *inv, Mat4::identity()));

  const gfx::Mat3 scaled = Mat4::scale({2, 3, 4}).upper3();
  const auto inv3 = gfx::inverse(scaled);
  GFX_CHECK(inv3.has_value());
  if (inv3) {
    GFX_CHECK(gfx::nearly(inv3->transform_vector({2, 3, 4}), Vec3{1, 1, 1}));
    const gfx::Mat3 nmat = gfx::normal_matrix(Mat4::scale({2, 1, 1}));
    GFX_CHECK(gfx::nearly(nmat.transform_vector({1, 1, 0}), Vec3{0.5f, 1, 0}));
  }

  const gfx::Mat3 local =
      gfx::Mat3::translation({150, 80}) * gfx::Mat3::rotation(gfx::radians(28.0f)) * gfx::Mat3::scale({1.4f, 0.7f});
  const gfx::Vec2 shape[] = {{0, -40}, {36, -10}, {20, 40}, {-20, 40}, {-36, -10}};

  gfx::Image image(420, 280, {16, 18, 26});
  std::vector<gfx::Vec2> plain;
  std::vector<gfx::Vec2> transformed;
  for (gfx::Vec2 p : shape) {
    plain.push_back({80 + p.x, 150 - p.y});
    const gfx::Vec2 q = local.transform_point(p);
    transformed.push_back({q.x, 150 - q.y});
  }
  draw_polygon(image, plain, {120, 144, 168});
  draw_polygon(image, transformed, {242, 176, 92});

  const auto path = gfx::output_file("04_matrices.bmp");
  if (!image.write_bmp(path) || gfx::failure_count() != 0) return 1;
  gfx::say("第 4 课完成：{}", path.string());
  return 0;
}
