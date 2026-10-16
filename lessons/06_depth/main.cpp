#include "log.hpp"
#include "raster.hpp"

namespace {

void draw_quad(gfx::Image& image, gfx::DepthBuffer& depth, const gfx::Mat4& vp, gfx::Vec3 p0, gfx::Vec3 p1,
               gfx::Vec3 p2, gfx::Vec3 p3, gfx::Vec3 color) {
  const auto triangle = [&](gfx::Vec3 a, gfx::Vec3 b, gfx::Vec3 c) {
    gfx::ClipVert verts[3];
    const gfx::Vec3 points[3] = {a, b, c};
    for (int i = 0; i < 3; ++i) {
      verts[i].world = points[i];
      verts[i].normal = {0, 0, 1};
      verts[i].color = color;
      verts[i].clip = vp * gfx::vec4(points[i], 1);
    }
    gfx::draw_clipped_triangle(image, depth, verts[0], verts[1], verts[2],
                               [](const gfx::Varying& frag) { return frag.color; });
  };
  triangle(p0, p1, p2);
  triangle(p0, p2, p3);
}

}  // namespace

int main() {
  constexpr int kWidth = 360;
  constexpr int kHeight = 270;
  gfx::Image image(kWidth, kHeight, {18, 20, 28});
  gfx::DepthBuffer depth(kWidth, kHeight);

  gfx::Camera camera;
  camera.eye = {0, 0, 4};
  camera.target = {0, 0, 0};
  camera.fov_y_degrees = 40;
  const gfx::Mat4 vp = gfx::view_projection(camera, static_cast<float>(kWidth) / kHeight);

  // 蓝色更远，红色更近。中心像素必须是红色，否则深度测试方向反了。
  draw_quad(image, depth, vp, {-1.4f, -1.2f, -1.2f}, {1.4f, -1.2f, -1.2f}, {1.4f, 1.2f, -1.2f},
            {-1.4f, 1.2f, -1.2f}, {0.15f, 0.35f, 0.95f});
  draw_quad(image, depth, vp, {-0.55f, -0.45f, 0.4f}, {0.7f, -0.35f, 0.2f}, {0.45f, 0.6f, 0.2f},
            {-0.7f, 0.45f, 0.5f}, {0.95f, 0.22f, 0.18f});

  const gfx::Color red = gfx::linear_to_color({0.95f, 0.22f, 0.18f});
  const gfx::Color blue = gfx::linear_to_color({0.15f, 0.35f, 0.95f});
  const gfx::Color center = image.get(kWidth / 2, kHeight / 2);
  int blue_pixels = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (image.get(x, y) == blue) ++blue_pixels;
    }
  }
  if (center != red || blue_pixels < 100) {
    gfx::say_error("深度测试没有让近处的红色盖住蓝色，中心像素是 {}, {}, {}，蓝色像素 {}", center.r, center.g,
                   center.b, blue_pixels);
    return 1;
  }

  const auto path = gfx::output_file("06_depth.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 6 课完成：{}", path.string());
  return 0;
}
