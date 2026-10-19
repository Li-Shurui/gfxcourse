#include "log.hpp"
#include "pathtracer.hpp"
#include "rasterizer.hpp"

#include <chrono>
#include <filesystem>
#include <string>

int main(int argc, char** argv) {
  const bool preview = gfx::has_flag(argc, argv, "--preview");
  // 第一个非选项参数是场景路径（默认 cornell），第二个是输出文件名前缀。
  std::string scene_arg;
  std::string prefix;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (!arg.empty() && arg[0] == '-') continue;
    if (scene_arg.empty()) scene_arg = arg;
    else if (prefix.empty()) prefix = arg;
  }
  const int width = preview ? 160 : 480;
  const int height = preview ? 120 : 360;
  const int samples = preview ? 4 : 48;

  const auto scene_path = scene_arg.empty() ? gfx::root_dir() / "assets/scenes/cornell.scene"
                                            : gfx::root_dir() / scene_arg;
  const auto scene = gfx::load_scene(scene_path);
  if (!scene) {
    gfx::say_error("{}", scene.error());
    return 1;
  }
  // 不带参数时保持课程默认输出名 16_*.bmp；指定场景后输出 <前缀_><场景名>_*.bmp。
  std::string tag = "16";
  if (!scene_arg.empty()) {
    tag = std::filesystem::path(scene_arg).stem().string();
    if (!prefix.empty()) tag = prefix + "_" + tag;
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

  const auto raster_path = gfx::output_file(tag + "_raster.bmp");
  const auto path_path = gfx::output_file(tag + "_path.bmp");
  const auto compare_path = gfx::output_file(tag + "_compare.bmp");
  if (!raster.write_bmp(raster_path) || !traced.write_bmp(path_path) || !compare.write_bmp(compare_path)) return 1;
  gfx::say("渲染完成（{}）。左光栅、右路径追踪：", scene_path.filename().string());
  gfx::say("  {}", compare_path.string());
  gfx::say("路径追踪 {:.0f} ms，{} spp", ms, samples);
  return 0;
}
