#include "light.hpp"
#include "log.hpp"
#include "shade.hpp"
#include "texture.hpp"

#include <vector>

int main() {
  constexpr int kWidth = 640;
  constexpr int kHeight = 360;
  gfx::Image image(kWidth, kHeight, {18, 20, 28});
  gfx::DepthBuffer depth(kWidth, kHeight);
  gfx::Camera camera;
  camera.eye = {0, 0, 3.4f};
  camera.target = {0, 0, 0};
  camera.fov_y_degrees = 32;

  gfx::Texture checker = gfx::make_checker(4, 8, {240, 236, 220}, {40, 78, 120});
  const std::vector<gfx::PointLight> lights{{{0.0f, 1.2f, 4.0f}, {1, 1, 1}, 18.0f}};
  const gfx::Vec3 ambient = gfx::gray(0.08f);

  const auto draw_wall = [&](float x, gfx::Texture::Filter filter) {
    checker.filter = filter;
    gfx::Geometry wall;
    gfx::add_quad(wall, {-0.85f, -0.85f, 0}, {0.85f, -0.85f, 0}, {0.85f, 0.85f, 0}, {-0.85f, 0.85f, 0}, {0, 0, 1});
    const gfx::Mat4 model =
        gfx::Mat4::translation({x, 0, 0}) * gfx::Mat4::rotation({0, 1, 0}, gfx::radians(x < 0 ? 18.0f : -18.0f));
    gfx::draw_geometry(image, depth, camera, model, wall, [&](const gfx::Varying& frag) {
      const gfx::Vec3 albedo = checker.sample(frag.uv);
      return gfx::blinn_phong(frag.world, frag.normal, camera.eye, albedo, gfx::gray(0.03f), 24.0f, ambient, lights);
    });
  };
  draw_wall(-1.05f, gfx::Texture::Filter::Nearest);
  draw_wall(1.05f, gfx::Texture::Filter::Bilinear);

  const auto path = gfx::output_file("08_texture.bmp");
  if (!image.write_bmp(path)) return 1;

  const gfx::Color left = image.get(kWidth / 4, kHeight / 2);
  const gfx::Color right = image.get(3 * kWidth / 4, kHeight / 2);
  if (left == image.get(0, 0) || right == image.get(0, 0)) {
    gfx::say_error("纹理四边形没有画进画面");
    return 1;
  }
  gfx::say("第 8 课完成：{}（左最近邻，右双线性）", path.string());
  return 0;
}
