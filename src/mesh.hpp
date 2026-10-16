#pragma once

#include "shapes.hpp"

#include <expected>
#include <filesystem>
#include <exception>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace gfx {

struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
};

struct Surface {
  std::string material;
  std::vector<std::uint32_t> indices;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<Surface> surfaces;

  int triangle_count() const {
    int count = 0;
    for (const Surface& surface : surfaces) count += static_cast<int>(surface.indices.size() / 3);
    return count;
  }
};

inline Mesh mesh_from_geometry(const Geometry& geometry, float radius = 1) {
  Mesh mesh;
  mesh.vertices.reserve(geometry.positions.size());
  for (std::size_t i = 0; i < geometry.positions.size(); ++i) {
    mesh.vertices.push_back(
        {geometry.positions[i] * radius, geometry.normals[i], i < geometry.uvs.size() ? geometry.uvs[i] : Vec2{}});
  }
  Surface surface;
  surface.indices = geometry.indices;
  mesh.surfaces.push_back(std::move(surface));
  return mesh;
}

inline Mesh make_uv_sphere(int slices, int stacks, float radius = 1) {
  return mesh_from_geometry(make_sphere(slices, stacks), radius);
}

inline std::expected<Mesh, std::string> load_obj(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) return std::unexpected("无法打开 OBJ: " + path.string());

  std::vector<Vec3> positions;
  std::vector<Vec2> uvs;
  std::vector<Vec3> normals;
  Mesh mesh;
  std::vector<char> authored_normal;
  std::map<std::tuple<int, int, int>, std::uint32_t> unique;
  std::string material;
  bool any_missing_normal = false;

  const auto fix_index = [](int index, int count) {
    if (index > 0) return index - 1;
    if (index < 0) return count + index;
    return -1;
  };

  const auto surface_for = [&](const std::string& name) -> Surface& {
    if (!mesh.surfaces.empty() && mesh.surfaces.back().material == name) return mesh.surfaces.back();
    mesh.surfaces.push_back(Surface{name, {}});
    return mesh.surfaces.back();
  };

  const auto add_corner = [&](int position_index, int uv_index, int normal_index) -> std::uint32_t {
    const auto key = std::tuple{position_index, uv_index, normal_index};
    if (const auto found = unique.find(key); found != unique.end()) return found->second;
    Vertex vertex;
    vertex.position = positions.at(static_cast<std::size_t>(position_index));
    if (uv_index >= 0) vertex.uv = uvs.at(static_cast<std::size_t>(uv_index));
    const bool has_normal = normal_index >= 0;
    if (has_normal) vertex.normal = normals.at(static_cast<std::size_t>(normal_index));
    else any_missing_normal = true;
    const auto id = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(vertex);
    authored_normal.push_back(has_normal ? 1 : 0);
    unique.emplace(key, id);
    return id;
  };

  std::string line;
  int line_number = 0;
  try {
  while (std::getline(in, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const auto comment = line.find('#');
    if (comment != std::string::npos) line.erase(comment);
    std::stringstream stream(line);
    std::string tag;
    if (!(stream >> tag)) continue;
    if (tag == "v") {
      float x = 0, y = 0, z = 0;
      stream >> x >> y >> z;
      positions.push_back({x, y, z});
    } else if (tag == "vt") {
      float u = 0, v = 0;
      stream >> u >> v;
      uvs.push_back({u, v});
    } else if (tag == "vn") {
      float x = 0, y = 0, z = 0;
      stream >> x >> y >> z;
      normals.push_back({x, y, z});
    } else if (tag == "usemtl") {
      stream >> material;
    } else if (tag == "f") {
      std::vector<std::uint32_t> corners;
      std::string token;
      while (stream >> token) {
        int v = 0, vt = 0, vn = 0;
        const auto first = token.find('/');
        v = std::stoi(token.substr(0, first));
        if (first != std::string::npos) {
          const auto second = token.find('/', first + 1);
          const std::string uv_text = token.substr(first + 1, second == std::string::npos ? std::string::npos : second - first - 1);
          if (!uv_text.empty()) vt = std::stoi(uv_text);
          if (second != std::string::npos && second + 1 < token.size()) vn = std::stoi(token.substr(second + 1));
        }
        const int pi = fix_index(v, static_cast<int>(positions.size()));
        const int ti = fix_index(vt, static_cast<int>(uvs.size()));
        const int ni = fix_index(vn, static_cast<int>(normals.size()));
        if (pi < 0 || pi >= static_cast<int>(positions.size())) {
          return std::unexpected("OBJ 顶点下标越界，行 " + std::to_string(line_number));
        }
        corners.push_back(add_corner(pi, ti, ni));
      }
      if (corners.size() < 3) continue;
      Surface& surface = surface_for(material);
      for (std::size_t i = 1; i + 1 < corners.size(); ++i) {
        surface.indices.push_back(corners[0]);
        surface.indices.push_back(corners[i]);
        surface.indices.push_back(corners[i + 1]);
      }
    }
  }
  } catch (const std::exception& ex) {
    return std::unexpected(std::string("OBJ 解析失败，行 ") + std::to_string(line_number) + ": " + ex.what());
  }

  if (any_missing_normal) {
    std::vector<Vec3> accumulated(mesh.vertices.size());
    for (const Surface& surface : mesh.surfaces) {
      for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3) {
        const Vec3& a = mesh.vertices[surface.indices[i]].position;
        const Vec3& b = mesh.vertices[surface.indices[i + 1]].position;
        const Vec3& c = mesh.vertices[surface.indices[i + 2]].position;
        const Vec3 face = cross(b - a, c - a);
        for (int k = 0; k < 3; ++k) {
          const auto index = surface.indices[i + static_cast<std::size_t>(k)];
          if (!authored_normal[index]) accumulated[index] += face;
        }
      }
    }
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
      if (!authored_normal[i]) mesh.vertices[i].normal = normalize(accumulated[i]);
    }
  }

  if (mesh.surfaces.empty()) return std::unexpected("OBJ 里没有面: " + path.string());
  return mesh;
}

}  // namespace gfx
