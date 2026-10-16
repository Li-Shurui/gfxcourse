#include "light.hpp"
#include "log.hpp"
#include "mesh.hpp"
#include "paths.hpp"
#include "shade.hpp"

#include <vector>

int main() {
  const auto cube_file = gfx::load_obj(gfx::root_dir() / "assets/meshes/cube.obj");
  const auto sphere_file = gfx::load_obj(gfx::root_dir() / "assets/meshes/sphere.obj");
  const auto torus_file = gfx::load_obj(gfx::root_dir() / "assets/meshes/torus.obj");
  const auto plane_file = gfx::load_obj(gfx::root_dir() / "assets/meshes/plane.obj");
  if (!cube_file || !sphere_file || !torus_file || !plane_file) {
    const auto& failed = !cube_file ? cube_file : !sphere_file ? sphere_file : !torus_file ? torus_file : plane_file;
    gfx::say_error("{}", failed.error());
    return 1;
  }
  if (cube_file->triangle_count() != 12 || sphere_file->triangle_count() < 100 || torus_file->triangle_count() < 100) {
    gfx::say_error("网格三角面数量异常");
    return 1;
  }

  const auto to_geometry = [](const gfx::Mesh& mesh) {
    gfx::Geometry geometry;
    geometry.positions.reserve(mesh.vertices.size());
    geometry.normals.reserve(mesh.vertices.size());
    geometry.uvs.reserve(mesh.vertices.size());
    for (const gfx::Vertex& vertex : mesh.vertices) {
      geometry.positions.push_back(vertex.position);
      geometry.normals.push_back(vertex.normal);
      geometry.uvs.push_back(vertex.uv);
    }
    for (const gfx::Surface& surface : mesh.surfaces) {
      geometry.indices.insert(geometry.indices.end(), surface.indices.begin(), surface.indices.end());
    }
    return geometry;
  };

  constexpr int kWidth = 640;
  constexpr int kHeight = 480;
  gfx::Image image(kWidth, kHeight, {16, 18, 26});
  gfx::DepthBuffer depth(kWidth, kHeight);
  gfx::Camera camera;
  camera.eye = {2.8f, 1.8f, 3.6f};
  camera.target = {0.1f, 0.45f, 0.0f};
  camera.fov_y_degrees = 34;
  const std::vector<gfx::PointLight> lights{{{2.4f, 3.4f, 2.0f}, {1.0f, 0.96f, 0.9f}, 30.0f},
                                            {{-2.2f, 1.6f, 2.4f}, {0.6f, 0.72f, 1.0f}, 12.0f}};
  const gfx::Vec3 ambient = gfx::gray(0.04f);
  const auto shade = [&](const gfx::Varying& frag, gfx::Vec3 albedo, float shininess) {
    return gfx::blinn_phong(frag.world, frag.normal, camera.eye, albedo, gfx::gray(0.06f), shininess, ambient, lights);
  };

  gfx::draw_geometry(image, depth, camera, gfx::Mat4::scale({2.4f, 1, 2.4f}), to_geometry(*plane_file),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.62f, 0.63f, 0.6f}, 16); });
  gfx::draw_geometry(image, depth, camera,
                     gfx::Mat4::translation({-0.9f, 0.5f, 0.1f}) * gfx::Mat4::rotation({0, 1, 0}, gfx::radians(24)) *
                         gfx::Mat4::scale(gfx::gray(0.5f)),
                     to_geometry(*cube_file),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.75f, 0.32f, 0.22f}, 24); });
  gfx::draw_geometry(image, depth, camera, gfx::Mat4::translation({0.85f, 0.55f, -0.1f}) * gfx::Mat4::scale(gfx::gray(0.55f)),
                     to_geometry(*sphere_file),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.28f, 0.48f, 0.82f}, 48); });
  gfx::draw_geometry(image, depth, camera,
                     gfx::Mat4::translation({0.05f, 0.24f, 0.55f}) * gfx::Mat4::rotation({0, 1, 0}, gfx::radians(-20)),
                     to_geometry(*torus_file),
                     [&](const gfx::Varying& frag) { return shade(frag, {0.92f, 0.74f, 0.28f}, 96); });

  const auto path = gfx::output_file("09_mesh.bmp");
  if (!image.write_bmp(path)) return 1;
  gfx::say("第 9 课完成：{}", path.string());
  return 0;
}
