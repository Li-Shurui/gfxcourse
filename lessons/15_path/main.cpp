#include "log.hpp"
#include "pathtracer.hpp"

#include <chrono>

int main(int argc, char** argv) {
  const bool preview = gfx::has_flag(argc, argv, "--preview");
  const int width = preview ? 120 : 400;
  const int height = preview ? 90 : 300;
  const int samples = preview ? 4 : 64;

  const auto scene = gfx::load_scene(gfx::root_dir() / "assets/scenes/cornell.scene");
  if (!scene) {
    gfx::say_error("{}", scene.error());
    return 1;
  }
  const gfx::RWorld world = gfx::RWorld::from_scene(*scene);
  const gfx::Bvh bvh = gfx::Bvh::build(world.triangles);
  gfx::Image image(width, height);
  const auto start = std::chrono::steady_clock::now();
  gfx::render_path(world, bvh, scene->camera, image, samples);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

  double sum = 0;
  for (int y = height / 4; y < 3 * height / 4; ++y) {
    for (int x = width / 4; x < 3 * width / 4; ++x) {
      const gfx::Color pixel = image.get(x, y);
      sum += pixel.r + pixel.g + pixel.b;
    }
  }
  if (sum < 1000.0) {
    gfx::say_error("路径追踪结果过暗");
    return 1;
  }
  const auto path = gfx::output_file("15_path.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 15 课完成：{}（{}x{}，{} spp，{:.0f} ms）", path.string(), width, height, samples, ms);
  return 0;
}
