#pragma once

#include "raster.hpp"
#include "shapes.hpp"

namespace gfx {

template <class Shader>
void draw_geometry(Image& image, DepthBuffer& depth, const Camera& camera, const Mat4& model, const Geometry& geometry,
                   Shader&& shader) {
  const float aspect = static_cast<float>(image.width()) / static_cast<float>(image.height());
  const Mat4 view_proj = view_projection(camera, aspect);
  const Mat3 normals = normal_matrix(model);
  for (std::size_t i = 0; i + 2 < geometry.indices.size(); i += 3) {
    ClipVert verts[3];
    bool ok = true;
    for (int k = 0; k < 3; ++k) {
      const std::uint32_t index = geometry.indices[i + static_cast<std::size_t>(k)];
      if (index >= geometry.positions.size()) {
        ok = false;
        break;
      }
      verts[k].world = model.transform_point(geometry.positions[index]);
      verts[k].normal = normals.transform_vector(geometry.normals[index]);
      verts[k].uv = geometry.uvs[index];
      verts[k].color = {1, 1, 1};
      verts[k].clip = view_proj * vec4(verts[k].world, 1);
    }
    if (!ok) continue;
    draw_clipped_triangle(image, depth, verts[0], verts[1], verts[2], shader);
  }
}

}  // namespace gfx
