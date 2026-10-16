#include "color.hpp"
#include "log.hpp"
#include "sphere.hpp"

#include <vector>

int main() {
  gfx::Camera camera;
  camera.eye = {0, 0.35f, 2.4f};
  camera.target = {0, 0, 0};
  camera.fov_y_degrees = 40;

  const gfx::Ray center_ray = gfx::camera_ray(camera, 160.0f, 120.0f, 320, 240);
  if (center_ray.direction.z > -0.9f) {
    gfx::say_error("屏幕中心的光线没有朝前");
    return 1;
  }

  const std::vector<gfx::Sphere> spheres{
      {{0, 0, 0}, 0.55f, 0},
      {{-1.15f, 0.1f, -0.2f}, 0.45f, 0},
      {{1.05f, -0.05f, 0.15f}, 0.4f, 0},
  };

  constexpr int kWidth = 320;
  constexpr int kHeight = 240;
  gfx::Image image(kWidth, kHeight);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const gfx::Ray ray = gfx::camera_ray(camera, x + 0.5f, y + 0.5f, kWidth, kHeight);
      gfx::Vec3 color = gfx::sky_color(ray);
      float closest = 1e30f;
      for (const gfx::Sphere& sphere : spheres) {
        gfx::Hit hit;
        if (sphere.intersect(ray, 0.001f, closest, hit)) {
          closest = hit.t;
          color = hit.normal * 0.5f + gfx::gray(0.5f);
        }
      }
      gfx::write_linear(image, x, y, color);
    }
  }

  const gfx::Color mid = image.get(kWidth / 2, kHeight / 2);
  if (mid.b < 80) {
    gfx::say_error("中心球体没有被命中");
    return 1;
  }
  const auto path = gfx::output_file("11_normals.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 11 课完成：{}", path.string());
  return 0;
}
