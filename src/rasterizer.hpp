#pragma once

#include "scene.hpp"
#include "shade.hpp"

#include <algorithm>
#include <span>

namespace gfx {

inline Vec3 shade_fragment(const Varying& frag, const Material& material, const Scene& scene) {
  Vec3 albedo = material.albedo;
  if (material.texture >= 0) albedo = albedo * scene.textures[static_cast<std::size_t>(material.texture)].sample(frag.uv);
  if (material.type == MaterialType::Emissive) return material.emission;

  Vec3 diffuse = albedo;
  Vec3 specular = gray(0.04f);
  float shininess = 32.0f;
  // 光栅器没有真正的反射/折射，金属和玻璃仍保留一部分漫反射，形状才看得清。
  if (material.type == MaterialType::Metal) {
    diffuse = albedo * 0.45f;
    specular = albedo;
    shininess = std::lerp(24.0f, 192.0f, 1.0f - std::clamp(material.roughness, 0.0f, 1.0f));
  } else if (material.type == MaterialType::Dielectric) {
    diffuse = albedo * 0.55f;
    specular = gray(0.85f);
    shininess = 160.0f;
  }
  return blinn_phong(frag.world, frag.normal, scene.camera.eye, diffuse, specular, shininess, scene.ambient,
                     scene.point_lights) +
         material.emission;
}

inline void draw_mesh_instance(const Scene& scene, Image& image, DepthBuffer& depth, const Mesh& mesh,
                               const Transform& transform, int fallback_material) {
  const float aspect = static_cast<float>(image.width()) / static_cast<float>(std::max(image.height(), 1));
  const Mat4 view_proj = view_projection(scene.camera, aspect);
  const Mat4 model = transform.matrix();
  const Mat3 normals = normal_matrix(model);

  for (const Surface& surface : mesh.surfaces) {
    int material_index = fallback_material;
    if (!surface.material.empty()) {
      const auto found = scene.material_of.find(surface.material);
      if (found != scene.material_of.end()) material_index = found->second;
    }
    const Material& material = scene.material(material_index);
    for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3) {
      ClipVert verts[3];
      bool ok = true;
      for (int k = 0; k < 3; ++k) {
        const auto index = surface.indices[i + static_cast<std::size_t>(k)];
        if (index >= mesh.vertices.size()) {
          ok = false;
          break;
        }
        const Vertex& vertex = mesh.vertices[index];
        verts[k].world = model.transform_point(vertex.position);
        verts[k].normal = normals.transform_vector(vertex.normal);
        verts[k].uv = vertex.uv;
        verts[k].color = material.albedo;
        verts[k].clip = view_proj * vec4(verts[k].world, 1);
      }
      if (!ok) continue;
      draw_clipped_triangle(image, depth, verts[0], verts[1], verts[2],
                            [&](const Varying& frag) { return shade_fragment(frag, material, scene); });
    }
  }
}

inline void render_raster(const Scene& scene, Image& image) {
  image.clear(scene.background);
  DepthBuffer depth(image.width(), image.height());
  for (const MeshInstance& instance : scene.meshes) {
    draw_mesh_instance(scene, image, depth, instance.mesh, instance.transform, instance.material);
  }
  for (const SphereInstance& sphere : scene.spheres) {
    Transform transform;
    transform.translation = sphere.center;
    transform.scale = gray(sphere.radius);
    const Mesh mesh = make_uv_sphere(48, 28);
    draw_mesh_instance(scene, image, depth, mesh, transform, sphere.material);
  }
}

}  // namespace gfx
