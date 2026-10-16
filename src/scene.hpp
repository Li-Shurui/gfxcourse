#pragma once

#include "camera.hpp"
#include "light.hpp"
#include "mesh.hpp"
#include "paths.hpp"
#include "texture.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gfx {

enum class MaterialType { Diffuse, Metal, Dielectric, Emissive };

struct Material {
  std::string name;
  MaterialType type = MaterialType::Diffuse;
  Vec3 albedo{0.8f, 0.8f, 0.8f};
  float roughness = 0.4f;
  float ior = 1.5f;
  Vec3 emission;
  int texture = -1;
};

struct Transform {
  Vec3 translation;
  Vec3 scale{1, 1, 1};
  float rotate_y_degrees = 0;

  Mat4 matrix() const {
    return Mat4::translation(translation) * Mat4::rotation({0, 1, 0}, radians(rotate_y_degrees)) * Mat4::scale(scale);
  }
};

struct MeshInstance {
  Mesh mesh;
  Transform transform;
  int material = 0;
};

struct SphereInstance {
  Vec3 center;
  float radius = 1;
  int material = 0;
};

struct AreaLight {
  Vec3 corner;
  Vec3 edge_u{1, 0, 0};
  Vec3 edge_v{0, 0, 1};
  Vec3 emission{1, 1, 1};

  Vec3 normal() const { return normalize(cross(edge_u, edge_v)); }
  float area() const { return length(cross(edge_u, edge_v)); }
  Vec3 center() const { return corner + (edge_u + edge_v) * 0.5f; }
  Vec3 sample(float u, float v) const { return corner + edge_u * u + edge_v * v; }
};

struct Scene {
  Camera camera;
  Vec3 ambient = gray(0.03f);
  Color background{16, 18, 24};
  bool sky = false;
  std::vector<Material> materials;
  std::vector<Texture> textures;
  std::vector<MeshInstance> meshes;
  std::vector<SphereInstance> spheres;
  std::vector<PointLight> point_lights;
  std::vector<AreaLight> area_lights;
  std::unordered_map<std::string, int> material_of;

  const Material& material(int index) const { return materials.at(static_cast<std::size_t>(index)); }
};

inline std::filesystem::path resolve_asset(const std::string& text) {
  const std::filesystem::path path{text};
  return path.is_relative() ? root_dir() / path : path;
}

inline std::expected<Scene, std::string> load_scene(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) return std::unexpected("无法打开场景: " + path.string());

  std::vector<std::string> tokens;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const auto comment = line.find('#');
    if (comment != std::string::npos) line.erase(comment);
    std::stringstream stream(line);
    std::string token;
    while (stream >> token) tokens.push_back(token);
  }

  struct Cursor {
    const std::vector<std::string>& tokens;
    std::size_t index = 0;
    std::string error;
    bool need(std::size_t count) {
      if (!error.empty()) return false;
      if (index + count <= tokens.size()) return true;
      error = "场景参数不足，停在: " + (index < tokens.size() ? tokens[index] : std::string("文件结尾"));
      return false;
    }
    const std::string& take() { return tokens[index++]; }
    float number() {
      if (!need(1)) return 0;
      const std::string& token = take();
      try {
        std::size_t used = 0;
        const float value = std::stof(token, &used);
        if (used != token.size()) error = "无法解析数字: " + token;
        return value;
      } catch (const std::exception&) {
        error = "无法解析数字: " + token;
        return 0;
      }
    }
    Vec3 vec3() { return {number(), number(), number()}; }
  } cursor{tokens, 0, {}};

  Scene scene;
  std::unordered_map<std::string, int> textures;
  bool saw_camera = false;

  const auto material_index = [&](const std::string& name) -> int {
    const auto found = scene.material_of.find(name);
    if (found == scene.material_of.end()) {
      cursor.error = "未知材质: " + name;
      return 0;
    }
    return found->second;
  };

  while (cursor.error.empty() && cursor.index < tokens.size()) {
    const std::string command = cursor.take();
    if (command == "camera") {
      if (!cursor.need(10)) break;
      scene.camera.eye = cursor.vec3();
      scene.camera.target = cursor.vec3();
      scene.camera.up = cursor.vec3();
      scene.camera.fov_y_degrees = cursor.number();
      saw_camera = true;
    } else if (command == "ambient") {
      scene.ambient = cursor.vec3();
    } else if (command == "background") {
      const Vec3 color = cursor.vec3();
      scene.background = {static_cast<std::uint8_t>(color.x), static_cast<std::uint8_t>(color.y),
                          static_cast<std::uint8_t>(color.z)};
    } else if (command == "sky") {
      scene.sky = cursor.number() != 0.0f;
    } else if (command == "point_light") {
      PointLight light;
      light.position = cursor.vec3();
      light.color = cursor.vec3();
      light.intensity = cursor.number();
      if (cursor.error.empty()) scene.point_lights.push_back(light);
    } else if (command == "area_light") {
      AreaLight light;
      light.corner = cursor.vec3();
      light.edge_u = cursor.vec3();
      light.edge_v = cursor.vec3();
      light.emission = cursor.vec3();
      if (cursor.error.empty()) scene.area_lights.push_back(light);
    } else if (command == "material") {
      if (!cursor.need(2)) break;
      Material material;
      material.name = cursor.take();
      const std::string type = cursor.take();
      if (type == "diffuse") {
        material.type = MaterialType::Diffuse;
        material.albedo = cursor.vec3();
        if (!cursor.need(1)) break;
        const std::string texture = cursor.take();
        if (texture != "none") {
          if (const auto found = textures.find(texture); found != textures.end()) {
            material.texture = found->second;
          } else {
            auto image = Image::read_ppm(resolve_asset(texture));
            if (!image) {
              cursor.error = image.error();
              break;
            }
            Texture stored;
            stored.image = std::move(*image);
            material.texture = static_cast<int>(scene.textures.size());
            textures.emplace(texture, material.texture);
            scene.textures.push_back(std::move(stored));
          }
        }
      } else if (type == "metal") {
        material.type = MaterialType::Metal;
        material.albedo = cursor.vec3();
        material.roughness = cursor.number();
      } else if (type == "dielectric") {
        material.type = MaterialType::Dielectric;
        material.albedo = cursor.vec3();
        material.ior = cursor.number();
      } else if (type == "emissive") {
        material.type = MaterialType::Emissive;
        material.emission = cursor.vec3();
        material.albedo = material.emission;
      } else {
        cursor.error = "未知材质类型: " + type;
      }
      if (cursor.error.empty()) {
        if (scene.material_of.contains(material.name)) cursor.error = "材质重复: " + material.name;
        else {
          scene.material_of.emplace(material.name, static_cast<int>(scene.materials.size()));
          scene.materials.push_back(std::move(material));
        }
      }
    } else if (command == "mesh") {
      if (!cursor.need(9)) break;
      const std::string material_name = cursor.take();
      MeshInstance instance;
      instance.material = material_index(material_name);
      instance.transform.translation = cursor.vec3();
      instance.transform.scale = cursor.vec3();
      instance.transform.rotate_y_degrees = cursor.number();
      if (!cursor.need(1)) break;
      const auto file = resolve_asset(cursor.take());
      auto mesh = load_obj(file);
      if (!mesh) {
        cursor.error = mesh.error();
        break;
      }
      for (const Surface& surface : mesh->surfaces) {
        if (!surface.material.empty() && !scene.material_of.contains(surface.material)) {
          cursor.error = "OBJ 引用了未定义材质 " + surface.material + " (" + file.string() + ")";
        }
      }
      instance.mesh = std::move(*mesh);
      if (cursor.error.empty()) scene.meshes.push_back(std::move(instance));
    } else if (command == "sphere") {
      if (!cursor.need(5)) break;
      SphereInstance sphere;
      sphere.material = material_index(cursor.take());
      sphere.center = cursor.vec3();
      sphere.radius = cursor.number();
      if (cursor.error.empty()) scene.spheres.push_back(sphere);
    } else {
      cursor.error = "未知场景命令: " + command;
    }
  }

  if (cursor.error.empty() && !saw_camera) cursor.error = "场景缺少 camera";
  if (!cursor.error.empty()) return std::unexpected(cursor.error);
  return scene;
}

}  // namespace gfx
