#pragma once

#include "scene.hpp"
#include "sphere.hpp"
#include "trimesh.hpp"

namespace gfx {

struct RWorld {
  const Scene* scene = nullptr;
  std::vector<Sphere> spheres;
  std::vector<Tri> triangles;

  static RWorld from_scene(const Scene& source) {
    RWorld world;
    world.scene = &source;
    for (const SphereInstance& sphere : source.spheres) {
      world.spheres.push_back(Sphere{sphere.center, sphere.radius, sphere.material});
    }
    for (const MeshInstance& instance : source.meshes) {
      const Mat4 model = instance.transform.matrix();
      const Mat3 normals = normal_matrix(model);
      for (const Surface& surface : instance.mesh.surfaces) {
        int material = instance.material;
        if (!surface.material.empty()) {
          const auto found = source.material_of.find(surface.material);
          if (found != source.material_of.end()) material = found->second;
        }
        for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3) {
          Tri tri;
          tri.material = material;
          for (int k = 0; k < 3; ++k) {
            const Vertex& vertex = instance.mesh.vertices[surface.indices[i + static_cast<std::size_t>(k)]];
            tri.p[k] = model.transform_point(vertex.position);
            tri.n[k] = normalize(normals.transform_vector(vertex.normal));
            tri.uv[k] = vertex.uv;
          }
          world.triangles.push_back(tri);
        }
      }
    }
    return world;
  }

  const Material& material(int index) const { return scene->material(index); }
  Vec3 shade_albedo(const Material& mat, Vec2 uv) const {
    Vec3 albedo = mat.albedo;
    if (mat.texture >= 0) albedo = albedo * scene->textures[static_cast<std::size_t>(mat.texture)].sample(uv);
    return albedo;
  }
  const std::vector<PointLight>& lights() const { return scene->point_lights; }
  Vec3 ambient() const { return scene->ambient; }
  Vec3 miss(const Ray& ray) const { return scene->sky ? sky_color(ray) : srgb_to_linear(scene->background); }

  bool closest_hit(const Ray& ray, float tmin, float tmax, Hit& hit) const {
    bool found = false;
    for (const Sphere& sphere : spheres) {
      if (sphere.intersect(ray, tmin, tmax, hit)) {
        found = true;
        tmax = hit.t;
      }
    }
    for (const Tri& tri : triangles) {
      if (intersect_triangle(ray, tri, tmin, tmax, hit)) {
        found = true;
        tmax = hit.t;
      }
    }
    return found;
  }

  bool occluded(const Ray& ray, float tmin, float tmax) const {
    float start = tmin;
    for (int guard = 0; guard < 12 && start < tmax; ++guard) {
      Hit hit;
      if (!closest_hit(ray, start, tmax, hit)) return false;
      if (material(hit.material).type != MaterialType::Dielectric) return true;
      start = hit.t + 1e-3f;
    }
    return false;
  }
};

}  // namespace gfx
