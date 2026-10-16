#include "camera.hpp"
#include "check.hpp"
#include "log.hpp"
#include "screen_line.hpp"

namespace {

void draw_world_line(gfx::Image& image, const gfx::Mat4& vp, gfx::Vec3 a, gfx::Vec3 b, gfx::Color color) {
  gfx::Vec4 ca = vp * gfx::vec4(a, 1);
  gfx::Vec4 cb = vp * gfx::vec4(b, 1);
  if (!gfx::clip_line_near(ca, cb)) return;
  float x0 = 0;
  float y0 = 0;
  float x1 = 0;
  float y1 = 0;
  if (!gfx::project_to_screen(ca, image.width(), image.height(), x0, y0)) return;
  if (!gfx::project_to_screen(cb, image.width(), image.height(), x1, y1)) return;
  gfx::draw_screen_line(image, x0, y0, x1, y1, color);
}

}  // namespace

int main() {
  gfx::Camera camera;
  camera.eye = {0, 0, 5};
  camera.target = {0, 0, 0};
  camera.fov_y_degrees = 90;
  const gfx::Mat4 vp = gfx::view_projection(camera, 1.0f);
  const gfx::Vec4 origin = vp * gfx::vec4({0, 0, 0}, 1);
  GFX_CHECK(origin.w > 0);
  GFX_CHECK(gfx::nearly(origin.x / origin.w, 0.0f));
  GFX_CHECK(gfx::nearly(origin.y / origin.w, 0.0f));

  const gfx::Vec3 eye_space = gfx::view_matrix(camera).transform_point({0, 0, 0});
  GFX_CHECK(gfx::nearly(eye_space, gfx::Vec3{0, 0, -5}));

  constexpr int kWidth = 480;
  constexpr int kHeight = 360;
  gfx::Image image(kWidth, kHeight, {12, 16, 24});
  gfx::Camera view;
  view.eye = {2.6f, 1.8f, 3.4f};
  view.target = {0, 0, 0};
  view.fov_y_degrees = 40;
  const gfx::Mat4 model = gfx::Mat4::rotation({0, 1, 0}, gfx::radians(28.0f));
  const gfx::Mat4 cube_vp = gfx::view_projection(view, static_cast<float>(kWidth) / kHeight) * model;

  const gfx::Vec3 corners[8] = {
      {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
      {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1},
  };
  const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                            {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
  for (const auto& edge : edges) {
    draw_world_line(image, cube_vp, corners[edge[0]], corners[edge[1]], {236, 228, 210});
  }
  draw_world_line(image, cube_vp, {0, 0, 0}, {1.6f, 0, 0}, {220, 70, 70});
  draw_world_line(image, cube_vp, {0, 0, 0}, {0, 1.6f, 0}, {70, 200, 110});
  draw_world_line(image, cube_vp, {0, 0, 0}, {0, 0, 1.6f}, {80, 140, 220});

  const gfx::Color background = image.get(0, 0);
  int inked = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (image.get(x, y) != background) ++inked;
    }
  }
  GFX_CHECK(inked > 200);

  const auto path = gfx::output_file("05_wireframe.bmp");
  if (!image.write_bmp(path) || gfx::failure_count() != 0) return 1;
  gfx::say("第 5 课完成：{}", path.string());
  return 0;
}
