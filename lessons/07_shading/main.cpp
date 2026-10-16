#include "check.hpp"
#include "light.hpp"
#include "log.hpp"
#include "shade.hpp"

#include <vector>

int main() {
  const gfx::PointLight key{{3.5f, 4.5f, 2.5f}, {1.0f, 0.95f, 0.85f}, 22.0f};
  const gfx::PointLight fill{{-3.0f, 2.2f, 3.0f}, {0.55f, 0.7f, 1.0f}, 14.0f};
  const std::vector<gfx::PointLight> lights{key, fill};
  const gfx::Vec3 ambient = gfx::gray(0.035f);
  const gfx::Vec3 lit = gfx::blinn_phong({0, 1, 0}, {0, 1, 0}, {3, 2, 4}, {0.7f, 0.3f, 0.2f}, gfx::gray(0.04f), 32,
                                         ambient, lights);
  const gfx::Vec3 dark = gfx::blinn_phong({0, 1, 0}, {0, -1, 0}, {3, 2, 4}, {0.7f, 0.3f, 0.2f}, gfx::gray(0.04f), 32,
                                          ambient, lights);
  GFX_CHECK(lit.x + lit.y + lit.z > dark.x + dark.y + dark.z + 0.05f);

  constexpr int kWidth = 640;
  constexpr int kHeight = 480;
  gfx::Image image(kWidth, kHeight, {16, 18, 26});
  gfx::DepthBuffer depth(kWidth, kHeight);
  gfx::Camera camera;
  camera.eye = {3.1f, 2.3f, 4.0f};
  camera.target = {0.0f, 0.7f, 0.0f};
  camera.fov_y_degrees = 34;

  const auto shade = [&](const gfx::Varying& frag, gfx::Vec3 albedo) {
    return gfx::blinn_phong(frag.world, frag.normal, camera.eye, albedo, gfx::gray(0.045f), 48.0f, ambient, lights);
  };

  gfx::draw_geometry(image, depth, camera, gfx::Mat4::identity(), gfx::make_plane(4.0f),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.62f, 0.64f, 0.66f}); });
  const gfx::Mat4 sphere = gfx::Mat4::translation({0.55f, 1.0f, 0.1f});
  gfx::draw_geometry(image, depth, camera, sphere, gfx::make_sphere(40, 24),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.25f, 0.45f, 0.85f}); });
  const gfx::Mat4 cube = gfx::Mat4::translation({-1.15f, 0.62f, 0.25f}) *
                         gfx::Mat4::rotation({0, 1, 0}, gfx::radians(22.0f)) * gfx::Mat4::scale(gfx::gray(0.62f));
  gfx::draw_geometry(image, depth, camera, cube, gfx::make_cube(),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.78f, 0.32f, 0.2f}); });

  const gfx::Color background = image.get(0, 0);
  int drawn = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (image.get(x, y) != background) ++drawn;
    }
  }
  GFX_CHECK(drawn > kWidth * kHeight / 10);

  const auto path = gfx::output_file("07_shading.bmp");
  if (!image.write_bmp(path) || gfx::failure_count() != 0) return 1;
  gfx::say("第 7 课完成：{}", path.string());
  return 0;
}
