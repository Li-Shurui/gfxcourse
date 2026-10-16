#include "log.hpp"
#include "whitted.hpp"
#include "world.hpp"

#include <chrono>

int main() {
  const auto scene = gfx::load_scene(gfx::root_dir() / "assets/scenes/torus.scene");
  if (!scene) {
    gfx::say_error("{}", scene.error());
    return 1;
  }
  const gfx::RWorld world = gfx::RWorld::from_scene(*scene);
  constexpr int kWidth = 360;
  constexpr int kHeight = 270;
  gfx::Image image(kWidth, kHeight);
  const auto start = std::chrono::steady_clock::now();
  gfx::render_whitted(image, scene->camera, world, 4);
  const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

  int drawn = 0;
  const gfx::Color corner = image.get(0, 0);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (image.get(x, y) != corner) ++drawn;
    }
  }
  if (drawn < 200) {
    gfx::say_error("网格光追几乎没有命中");
    return 1;
  }
  const auto path = gfx::output_file("13_mesh_ray.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 13 课完成：{}（{:.0f} ms，暴力 {} 个三角形）", path.string(), ms, world.triangles.size());
  return 0;
}
