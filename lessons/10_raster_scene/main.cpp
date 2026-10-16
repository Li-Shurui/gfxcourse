#include "log.hpp"
#include "rasterizer.hpp"

int main() {
  const auto scene = gfx::load_scene(gfx::root_dir() / "assets/scenes/studio.scene");
  if (!scene) {
    gfx::say_error("{}", scene.error());
    return 1;
  }

  constexpr int kWidth = 640;
  constexpr int kHeight = 480;
  gfx::Image image(kWidth, kHeight);
  gfx::render_raster(*scene, image);

  const gfx::Color background = scene->background;
  int drawn = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      if (image.get(x, y) != background) ++drawn;
    }
  }
  if (drawn < kWidth * kHeight / 8) {
    gfx::say_error("场景几乎是空的，画了 {} 个像素", drawn);
    return 1;
  }

  const auto path = gfx::output_file("10_raster_scene.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 10 课完成：{}", path.string());
  return 0;
}
