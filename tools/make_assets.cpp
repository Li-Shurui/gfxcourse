#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

struct Vec2 {
  float x = 0, y = 0;
};

Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 mul(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 normalize(Vec3 v) {
  const float len = std::sqrt(dot(v, v));
  return len > 1e-8f ? mul(v, 1.0f / len) : Vec3{};
}

class ObjWriter {
 public:
  int add_vertex(Vec3 p, Vec3 n, Vec2 uv) {
    positions.push_back(p);
    normals.push_back(normalize(n));
    uvs.push_back(uv);
    return static_cast<int>(positions.size());
  }

  void triangle(int a, int b, int c, const std::string& material) {
    faces.push_back({a, b, c, material});
  }

  void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 want_normal, const std::string& material) {
    const Vec3 n = normalize(want_normal);
    const int ia = add_vertex(a, n, {0, 0});
    const int ib = add_vertex(b, n, {1, 0});
    const int ic = add_vertex(c, n, {1, 1});
    const int id = add_vertex(d, n, {0, 1});
    const bool flip = dot(cross(sub(b, a), sub(c, a)), want_normal) < 0.0f;
    if (!flip) {
      triangle(ia, ib, ic, material);
      triangle(ia, ic, id, material);
    } else {
      triangle(ia, ic, ib, material);
      triangle(ia, id, ic, material);
    }
  }

  void quad_uv(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 want_normal, Vec2 ua, Vec2 ub, Vec2 uc, Vec2 ud,
               const std::string& material) {
    const Vec3 n = normalize(want_normal);
    const int ia = add_vertex(a, n, ua);
    const int ib = add_vertex(b, n, ub);
    const int ic = add_vertex(c, n, uc);
    const int id = add_vertex(d, n, ud);
    const bool flip = dot(cross(sub(b, a), sub(c, a)), want_normal) < 0.0f;
    if (!flip) {
      triangle(ia, ib, ic, material);
      triangle(ia, ic, id, material);
    } else {
      triangle(ia, ic, ib, material);
      triangle(ia, id, ic, material);
    }
  }

  void save(const std::filesystem::path& path) const {
    std::ofstream out(path);
    out << "# gfxcourse mesh\n";
    for (Vec3 p : positions) out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (Vec2 uv : uvs) out << "vt " << uv.x << ' ' << uv.y << '\n';
    for (Vec3 n : normals) out << "vn " << n.x << ' ' << n.y << ' ' << n.z << '\n';
    std::string material;
    for (const Face& face : faces) {
      if (face.material != material) {
        material = face.material;
        if (!material.empty()) out << "usemtl " << material << '\n';
      }
      out << "f " << face.v[0] << '/' << face.v[0] << '/' << face.v[0] << ' ' << face.v[1] << '/' << face.v[1]
          << '/' << face.v[1] << ' ' << face.v[2] << '/' << face.v[2] << '/' << face.v[2] << '\n';
    }
  }

 private:
  struct Face {
    int v[3];
    std::string material;
    Face(int a, int b, int c, std::string mat) : v{a, b, c}, material(std::move(mat)) {}
  };
  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> uvs;
  std::vector<Face> faces;
};

void add_box(ObjWriter& writer, Vec3 mn, Vec3 mx, const std::string& material) {
  writer.quad({mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, {mx.x, mx.y, mx.z}, {mx.x, mx.y, mn.z}, {1, 0, 0}, material);
  writer.quad({mn.x, mn.y, mx.z}, {mn.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, {mn.x, mx.y, mx.z}, {-1, 0, 0}, material);
  writer.quad({mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z}, {mx.x, mx.y, mn.z}, {mn.x, mx.y, mn.z}, {0, 1, 0}, material);
  writer.quad({mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z}, {0, -1, 0}, material);
  writer.quad({mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z}, {mx.x, mx.y, mx.z}, {mn.x, mx.y, mx.z}, {0, 0, 1}, material);
  writer.quad({mx.x, mn.y, mn.z}, {mn.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, {0, 0, -1}, material);
}

void write_grid_mesh(const std::filesystem::path& path, int slices, int stacks, auto&& position_of) {
  ObjWriter writer;
  for (int y = 0; y <= stacks; ++y) {
    for (int x = 0; x <= slices; ++x) {
      const float u = static_cast<float>(x) / static_cast<float>(slices);
      const float v = static_cast<float>(y) / static_cast<float>(stacks);
      Vec3 position;
      Vec3 normal;
      position_of(u, v, position, normal);
      writer.add_vertex(position, normal, {u, 1.0f - v});
    }
  }
  const auto id = [slices](int x, int y) { return y * (slices + 1) + x + 1; };
  for (int y = 0; y < stacks; ++y) {
    for (int x = 0; x < slices; ++x) {
      const int i00 = id(x, y);
      const int i10 = id(x + 1, y);
      const int i01 = id(x, y + 1);
      const int i11 = id(x + 1, y + 1);
      writer.triangle(i00, i11, i10, "");
      writer.triangle(i00, i01, i11, "");
    }
  }
  writer.save(path);
}

void write_ppm(const std::filesystem::path& path, int width, int height, const std::vector<std::uint8_t>& rgb) {
  std::ofstream out(path, std::ios::binary);
  out << "P6\n" << width << ' ' << height << "\n255\n";
  out.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
}

void write_text(const std::filesystem::path& path, const std::string& text) {
  std::ofstream out(path);
  out << text;
}

}  // namespace

int main() {
#ifndef GFX_ROOT
  const std::filesystem::path root = ".";
#else
  const std::filesystem::path root = GFX_ROOT;
#endif
  const auto mesh_dir = root / "assets/meshes";
  const auto texture_dir = root / "assets/textures";
  const auto scene_dir = root / "assets/scenes";
  std::filesystem::create_directories(mesh_dir);
  std::filesystem::create_directories(texture_dir);
  std::filesystem::create_directories(scene_dir);

  {
    ObjWriter cube;
    add_box(cube, {-1, -1, -1}, {1, 1, 1}, "");
    cube.save(mesh_dir / "cube.obj");
  }
  {
    ObjWriter plane;
    plane.quad_uv({-1, 0, -1}, {1, 0, -1}, {1, 0, 1}, {-1, 0, 1}, {0, 1, 0}, {0, 0}, {4, 0}, {4, 4}, {0, 4}, "");
    plane.save(mesh_dir / "plane.obj");
  }
  write_grid_mesh(mesh_dir / "sphere.obj", 48, 32, [](float u, float v, Vec3& position, Vec3& normal) {
    const float phi = v * 3.14159265f;
    const float theta = u * 6.2831853f;
    normal = {std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
    position = normal;
  });
  write_grid_mesh(mesh_dir / "torus.obj", 56, 28, [](float u, float v, Vec3& position, Vec3& normal) {
    constexpr float kMajor = 0.52f;
    constexpr float kMinor = 0.18f;
    const float theta = u * 6.2831853f;
    const float phi = v * 6.2831853f;
    const Vec3 center{kMajor * std::cos(theta), 0, kMajor * std::sin(theta)};
    normal = {std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta)};
    position = add(center, mul(normal, kMinor));
  });

  {
    ObjWriter cornell;
    cornell.quad({-1, 0, -1}, {1, 0, -1}, {1, 0, 1}, {-1, 0, 1}, {0, 1, 0}, "white");
    cornell.quad({-1, 2, 1}, {1, 2, 1}, {1, 2, -1}, {-1, 2, -1}, {0, -1, 0}, "white");
    cornell.quad({-1, 0, -1}, {-1, 2, -1}, {1, 2, -1}, {1, 0, -1}, {0, 0, 1}, "white");
    cornell.quad({-1, 0, 1}, {-1, 0, -1}, {-1, 2, -1}, {-1, 2, 1}, {1, 0, 0}, "red");
    cornell.quad({1, 0, -1}, {1, 0, 1}, {1, 2, 1}, {1, 2, -1}, {-1, 0, 0}, "green");
    add_box(cornell, {-0.08f, 0, -0.95f}, {0.52f, 0.58f, -0.38f}, "white");
    const Vec3 corner{-0.22f, 1.995f, -0.22f};
    const Vec3 edge_u{0.44f, 0, 0};
    const Vec3 edge_v{0, 0, 0.44f};
    cornell.quad(corner, add(corner, edge_u), add(add(corner, edge_u), edge_v), add(corner, edge_v), {0, -1, 0},
                 "light");
    cornell.save(mesh_dir / "cornell.obj");
  }

  {
    constexpr int kSize = 128;
    std::vector<std::uint8_t> checker(static_cast<std::size_t>(kSize * kSize * 3));
    for (int y = 0; y < kSize; ++y) {
      for (int x = 0; x < kSize; ++x) {
        const bool on = ((x / 16) + (y / 16)) % 2 == 0;
        const std::size_t i = static_cast<std::size_t>(y * kSize + x) * 3;
        checker[i] = on ? 236 : 42;
        checker[i + 1] = on ? 228 : 74;
        checker[i + 2] = on ? 210 : 92;
      }
    }
    write_ppm(texture_dir / "checker.ppm", kSize, kSize, checker);

    std::vector<std::uint8_t> tiles(static_cast<std::size_t>(kSize * kSize * 3));
    for (int y = 0; y < kSize; ++y) {
      for (int x = 0; x < kSize; ++x) {
        const int gx = x % 32;
        const int gy = y % 32;
        const bool grout = gx < 2 || gy < 2;
        std::uint32_t hash = static_cast<std::uint32_t>(x / 32 * 374761393 + y / 32 * 668265263);
        hash = (hash ^ (hash >> 13)) * 1274126177u;
        const int jitter = static_cast<int>(hash & 15u);
        const std::size_t i = static_cast<std::size_t>(y * kSize + x) * 3;
        if (grout) {
          tiles[i] = 186;
          tiles[i + 1] = 176;
          tiles[i + 2] = 160;
        } else {
          tiles[i] = static_cast<std::uint8_t>(214 + jitter);
          tiles[i + 1] = static_cast<std::uint8_t>(206 + jitter / 2);
          tiles[i + 2] = static_cast<std::uint8_t>(190 + jitter / 3);
        }
      }
    }
    write_ppm(texture_dir / "tiles.ppm", kSize, kSize, tiles);
  }

  write_text(scene_dir / "studio.scene", R"(# 光栅器静物。材质必须写在 mesh 之前。
# mesh 材质 tx ty tz  sx sy sz  绕Y旋转角度  模型路径
camera 2.55 1.85 3.55  0.05 0.42 0.05  0 1 0  32
ambient 0.07 0.075 0.08
background 18 22 30
sky 0

point_light 2.4 3.6 2.2  1 0.96 0.88  34
point_light -2.6 2.1 2.6  0.65 0.75 1  16

material clay diffuse 0.74 0.30 0.18 none
material glaze diffuse 0.28 0.46 0.82 none
material gold metal 0.94 0.76 0.32 0.12
material glass dielectric 0.98 0.99 0.98 1.5
material tile diffuse 0.92 0.92 0.9 assets/textures/tiles.ppm

mesh tile 0 0 0  2.8 1 2.8  0  assets/meshes/plane.obj
mesh clay -1.25 0.5 -0.55  0.5 0.5 0.5  24  assets/meshes/cube.obj
mesh glaze 1.25 0.52 -0.65  0.52 0.52 0.52  0  assets/meshes/sphere.obj
mesh gold 0.05 0.18 0.72  1 1 1  -12  assets/meshes/torus.obj
mesh glass -0.2 0.2 -0.05  0.2 0.2 0.2  0  assets/meshes/sphere.obj
)");

  write_text(scene_dir / "torus.scene", R"(# 第 13 课：用 Whitted 光线追踪一个网格。
camera 2.15 0.95 2.55  0 0.32 0.05  0 1 0  34
ambient 0.03 0.03 0.035
background 18 22 30
sky 1

point_light 1.6 2.6 1.4  1 0.97 0.9  28
point_light -2.2 1.8 1.2  0.7 0.75 1  10

material floor diffuse 0.72 0.7 0.64 assets/textures/checker.ppm
material gold metal 0.95 0.78 0.34 0.04
material clay diffuse 0.72 0.24 0.16 none

mesh floor 0 0 0  2.6 1 2.6  0  assets/meshes/plane.obj
mesh gold 0.15 0.36 0.05  1.05 1.05 1.05  24  assets/meshes/torus.obj
sphere clay -0.95 0.38 0.15  0.38
)");

  write_text(scene_dir / "cornell.scene", R"(# 康奈尔盒。光栅器用 point_light，路径追踪用 area_light 和灯片网格。
camera 0 0.95 3.2  0 0.86 0  0 1 0  36
ambient 0.025 0.025 0.028
background 0 0 0
sky 0

point_light 0 1.75 2.4  1 1 1  10
area_light -0.22 1.995 -0.22  0.44 0 0  0 0 0.44  22 22 22

material red diffuse 0.72 0.12 0.08 none
material green diffuse 0.12 0.62 0.16 none
material white diffuse 0.73 0.73 0.73 none
material light emissive 22 22 22
material gold metal 0.92 0.84 0.62 0.05
material glass dielectric 0.98 0.98 0.98 1.5

mesh white 0 0 0  1 1 1  0  assets/meshes/cornell.obj
sphere gold -0.45 0.32 0.28  0.32
sphere glass 0.40 0.32 0.05  0.32
)");

  std::cout << "素材已写到 " << (root / "assets").string() << '\n';
  return 0;
}
