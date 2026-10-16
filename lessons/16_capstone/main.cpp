#include "log.hpp"
#include "pathtracer.hpp"
#include "rasterizer.hpp"

#include <chrono>

int main(int argc, char** argv) {
  const bool preview = gfx::has_flag(argc, argv, "--preview");
  const int width = preview ? 160 : 480;
  const int height = preview ? 120 : 360;
  const int samples = preview ? 4 : 48;

  const auto scene = gfx::load_scene(gfx::root_dir() / "assets/scenes/cornell.scene");
  if (!scene) {
    gfx::say_error("{}", scene.error());
    return 1;
  }

  gfx::Image raster(width, height);
  gfx::render_raster(*scene, raster);

  const gfx::RWorld world = gfx::RWorld::from_scene(*scene);
  const gfx::Bvh bvh = gfx::Bvh::build(world.triangles);
  gfx::Image traced(width, height);
  const auto start = std::chrono::steady_clock::now();
  gfx::render_path(world, bvh, scene->camera, traced, samples);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

  gfx::Image compare(width * 2 + 8, height, {236, 232, 220});
  compare.blit(raster, 0, 0);
  compare.blit(traced, width + 8, 0);

  const auto raster_path = gfx::output_file("16_raster.bmp");
  const auto path_path = gfx::output_file("16_path.bmp");
  const auto compare_path = gfx::output_file("16_compare.bmp");
  if (!raster.write_bmp(raster_path) || !traced.write_bmp(path_path) || !compare.write_bmp(compare_path)) return 1;
  gfx::say("第 16 课完成。左光栅、右路径追踪：");
  gfx::say("  {}", compare_path.string());
  gfx::say("路径追踪 {:.0f} ms，{} spp", ms, samples);
  return 0;
}
