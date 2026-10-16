#include "log.hpp"
#include "sphere_world.hpp"
#include "whitted.hpp"

#include <utility>

int main() {
  gfx::SphereWorld world;
  const auto add_material = [&](std::string name, gfx::MaterialType type, gfx::Vec3 albedo, float roughness = 0.4f,
                                float ior = 1.5f) {
    gfx::Material material;
    material.name = std::move(name);
    material.type = type;
    material.albedo = albedo;
    material.roughness = roughness;
    material.ior = ior;
    world.materials.push_back(std::move(material));
  };
  add_material("ground", gfx::MaterialType::Diffuse, {0.55f, 0.58f, 0.52f});
  add_material("clay", gfx::MaterialType::Diffuse, {0.72f, 0.22f, 0.16f});
  add_material("gold", gfx::MaterialType::Metal, {0.94f, 0.76f, 0.32f}, 0.08f);
  add_material("glass", gfx::MaterialType::Dielectric, {0.98f, 0.98f, 0.98f}, 0.0f, 1.5f);
  world.spheres.push_back({{0, -100.5f, -1}, 100.0f, 0});
  world.spheres.push_back({{-1.15f, 0, -1.1f}, 0.5f, 1});
  world.spheres.push_back({{1.2f, 0, -1.25f}, 0.5f, 2});
  world.spheres.push_back({{0.05f, 0, -1.0f}, 0.5f, 3});
  world.point_lights.push_back({{-1.5f, 4.5f, 2.2f}, {1, 1, 1}, 55.0f});

  gfx::Camera camera;
  camera.eye = {0.1f, 0.7f, 2.3f};
  camera.target = {0.05f, 0.15f, -1.0f};
  camera.fov_y_degrees = 34;

  constexpr int kWidth = 320;
  constexpr int kHeight = 240;
  gfx::Image image(kWidth, kHeight);
  gfx::render_whitted(image, camera, world, 5);

  int bright = 0;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const gfx::Color pixel = image.get(x, y);
      if (pixel.r + pixel.g + pixel.b > 80) ++bright;
    }
  }
  if (bright < 500) {
    gfx::say_error("Whitted 图像过暗");
    return 1;
  }
  const auto path = gfx::output_file("12_whitted.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 12 课完成：{}", path.string());
  return 0;
}
