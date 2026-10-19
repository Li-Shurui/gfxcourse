#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
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

// 沿 dir 方向放一个椭球：radii 是三个轴的半径，用于拼人偶的关节部件。
void add_ellipsoid(ObjWriter& writer, Vec3 center, Vec3 dir, Vec3 radii, int slices, int stacks,
                   const std::string& material) {
  const Vec3 y = normalize(dir);
  const Vec3 x = normalize(cross(std::fabs(y.y) < 0.999f ? Vec3{0, 1, 0} : Vec3{1, 0, 0}, y));
  const Vec3 z = cross(x, y);
  std::vector<int> ids;
  for (int j = 0; j <= stacks; ++j) {
    for (int i = 0; i <= slices; ++i) {
      const float u = static_cast<float>(i) / static_cast<float>(slices);
      const float v = static_cast<float>(j) / static_cast<float>(stacks);
      const float phi = v * 3.14159265f;
      const float theta = u * 6.2831853f;
      const Vec3 p{std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
      const Vec3 local{p.x * radii.x, p.y * radii.y, p.z * radii.z};
      const Vec3 nl{p.x / radii.x, p.y / radii.y, p.z / radii.z};
      const Vec3 world = add(center, add(add(mul(x, local.x), mul(y, local.y)), mul(z, local.z)));
      const Vec3 wn = add(add(mul(x, nl.x), mul(y, nl.y)), mul(z, nl.z));
      ids.push_back(writer.add_vertex(world, wn, {u, 1.0f - v}));
    }
  }
  const auto id = [slices](int i, int j) { return j * (slices + 1) + i; };
  for (int j = 0; j < stacks; ++j) {
    for (int i = 0; i < slices; ++i) {
      writer.triangle(ids[static_cast<std::size_t>(id(i, j))], ids[static_cast<std::size_t>(id(i + 1, j + 1))],
                      ids[static_cast<std::size_t>(id(i + 1, j))], material);
      writer.triangle(ids[static_cast<std::size_t>(id(i, j))], ids[static_cast<std::size_t>(id(i, j + 1))],
                      ids[static_cast<std::size_t>(id(i + 1, j + 1))], material);
    }
  }
}

// 部件拉长一点覆盖关节，重叠处渲染器本来就支持。
void add_limb(ObjWriter& writer, Vec3 a, Vec3 b, float radius, const std::string& material) {
  const Vec3 mid = mul(add(a, b), 0.5f);
  const Vec3 dir = sub(b, a);
  const float len = std::sqrt(dot(dir, dir));
  add_ellipsoid(writer, mid, dir, {radius, len * 0.58f, radius}, 28, 20, material);
}

void write_human_mesh(const std::filesystem::path& path) {
  ObjWriter human;
  // 鞋
  add_ellipsoid(human, {-0.09f, 0.045f, 0.04f}, {0, 0, 1}, {0.062f, 0.045f, 0.13f}, 28, 20, "shoes");
  add_ellipsoid(human, {0.09f, 0.045f, 0.04f}, {0, 0, 1}, {0.062f, 0.045f, 0.13f}, 28, 20, "shoes");
  // 小腿、大腿
  add_limb(human, {-0.09f, 0.09f, 0}, {-0.1f, 0.52f, 0}, 0.052f, "pants");
  add_limb(human, {0.09f, 0.09f, 0}, {0.1f, 0.52f, 0}, 0.052f, "pants");
  add_limb(human, {-0.1f, 0.52f, 0}, {-0.1f, 0.86f, 0}, 0.068f, "pants");
  add_limb(human, {0.1f, 0.52f, 0}, {0.1f, 0.86f, 0}, 0.068f, "pants");
  // 骨盆、躯干
  add_ellipsoid(human, {0, 0.9f, 0}, {0, 1, 0}, {0.135f, 0.1f, 0.095f}, 28, 20, "pants");
  add_limb(human, {0, 0.96f, 0}, {0, 1.3f, 0}, 0.15f, "shirt");
  // 大臂、小臂、手
  add_limb(human, {-0.165f, 1.27f, 0}, {-0.235f, 1.03f, 0.03f}, 0.045f, "shirt");
  add_limb(human, {0.165f, 1.27f, 0}, {0.235f, 1.03f, 0.03f}, 0.045f, "shirt");
  add_limb(human, {-0.235f, 1.03f, 0.03f}, {-0.255f, 0.79f, 0.07f}, 0.038f, "skin");
  add_limb(human, {0.235f, 1.03f, 0.03f}, {0.255f, 0.79f, 0.07f}, 0.038f, "skin");
  add_ellipsoid(human, {-0.26f, 0.73f, 0.08f}, {0, 1, 0}, {0.048f, 0.055f, 0.048f}, 28, 20, "skin");
  add_ellipsoid(human, {0.26f, 0.73f, 0.08f}, {0, 1, 0}, {0.048f, 0.055f, 0.048f}, 28, 20, "skin");
  // 脖子、头、眼睛
  add_limb(human, {0, 1.31f, 0}, {0, 1.4f, 0}, 0.042f, "skin");
  add_ellipsoid(human, {0, 1.5f, 0}, {0, 1, 0}, {0.088f, 0.105f, 0.092f}, 28, 20, "skin");
  add_ellipsoid(human, {-0.033f, 1.52f, 0.08f}, {0, 0, 1}, {0.013f, 0.013f, 0.013f}, 12, 8, "eye");
  add_ellipsoid(human, {0.033f, 1.52f, 0.08f}, {0, 0, 1}, {0.013f, 0.013f, 0.013f}, 12, 8, "eye");
  human.save(path);
}

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
area_light -1 3.8 -0.6  2 0 0  0 0 2  7 6.8 6.4

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

  // ---- 展示用例：更多网格 ----
  {
    // 四个侧面各用一种材质的锥体，专门测试 OBJ 的 usemtl 覆盖网格默认材质。
    ObjWriter pyramid;
    const Vec3 apex{0, 1.1f, 0};
    const Vec3 corners[4] = {{-0.6f, 0, -0.6f}, {0.6f, 0, -0.6f}, {0.6f, 0, 0.6f}, {-0.6f, 0, 0.6f}};
    const char* side_mats[4] = {"side_a", "side_b", "side_c", "side_d"};
    const Vec3 center{0, 0.28f, 0};
    for (int i = 0; i < 4; ++i) {
      Vec3 a = corners[i];
      Vec3 b = corners[(i + 1) % 4];
      Vec3 n = cross(sub(b, a), sub(apex, a));
      const Vec3 face_mid = mul(add(add(a, b), apex), 1.0f / 3.0f);
      if (dot(n, sub(face_mid, center)) < 0.0f) {
        std::swap(a, b);
        n = mul(n, -1.0f);
      }
      n = normalize(n);
      const int ia = pyramid.add_vertex(a, n, {0, 0});
      const int ib = pyramid.add_vertex(b, n, {1, 0});
      const int ic = pyramid.add_vertex(apex, n, {0.5f, 1});
      pyramid.triangle(ia, ib, ic, side_mats[i]);
    }
    pyramid.quad(corners[3], corners[2], corners[1], corners[0], {0, -1, 0}, "base");
    pyramid.save(mesh_dir / "pyramid.obj");
  }
  {
    // 六阶楼梯，测试大量盒体之间的相互遮挡。
    ObjWriter stairs;
    for (int i = 0; i < 6; ++i) {
      const float x0 = -1.35f + static_cast<float>(i) * 0.45f;
      add_box(stairs, {x0, 0, -0.6f}, {x0 + 0.45f, (static_cast<float>(i) + 1.0f) * 0.16f, 0.6f}, "");
    }
    stairs.save(mesh_dir / "stairs.obj");
  }
  write_grid_mesh(mesh_dir / "cone.obj", 48, 16, [](float u, float v, Vec3& position, Vec3& normal) {
    const float theta = u * 6.2831853f;
    const float radius = 0.55f * (1.0f - v);
    position = {radius * std::cos(theta), v * 1.1f, radius * std::sin(theta)};
    normal = {std::cos(theta), 0.5f, std::sin(theta)};
  });
  {
    // 竖直的墙（plane.obj 是水平的），用来展示纹理在不同朝向的面上。
    ObjWriter wall;
    wall.quad_uv({-1, 0, 0}, {1, 0, 0}, {1, 2, 0}, {-1, 2, 0}, {0, 0, 1}, {0, 0}, {2, 0}, {2, 2}, {0, 2}, "");
    wall.save(mesh_dir / "wall.obj");
  }

  // ---- 展示用例：更多纹理 ----
  {
    constexpr int kSize = 128;
    std::vector<std::uint8_t> gradient(static_cast<std::size_t>(kSize * kSize * 3));
    for (int y = 0; y < kSize; ++y) {
      for (int x = 0; x < kSize; ++x) {
        // 水平方向扫一遍色相，垂直方向改变明度，双线性和 sRGB 解码一眼可辨。
        const float hue = static_cast<float>(x) / static_cast<float>(kSize - 1) * 6.0f;
        const int sector = static_cast<int>(hue) % 6;
        const float f = hue - static_cast<float>(static_cast<int>(hue));
        const float rainbow[6][3] = {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}};
        const float left[3] = {rainbow[sector][0], rainbow[sector][1], rainbow[sector][2]};
        const float right[3] = {rainbow[(sector + 1) % 6][0], rainbow[(sector + 1) % 6][1],
                                rainbow[(sector + 1) % 6][2]};
        const float brightness = 1.0f - static_cast<float>(y) / static_cast<float>(kSize - 1) * 0.7f;
        const std::size_t i = static_cast<std::size_t>(y * kSize + x) * 3;
        for (int c = 0; c < 3; ++c) {
          const float value = (left[c] + (right[c] - left[c]) * f) * brightness;
          gradient[i + static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(value * 255.0f + 0.5f);
        }
      }
    }
    write_ppm(texture_dir / "gradient.ppm", kSize, kSize, gradient);

    std::vector<std::uint8_t> uvgrid(static_cast<std::size_t>(kSize * kSize * 3));
    for (int y = 0; y < kSize; ++y) {
      for (int x = 0; x < kSize; ++x) {
        const int cx = x / 16;
        const int cy = y / 16;
        const bool border = x % 16 < 1 || y % 16 < 1;
        std::uint8_t r = border ? 30 : (cx + cy) % 2 == 0 ? 220 : 170;
        std::uint8_t g = border ? 30 : (cx + cy) % 2 == 0 ? 220 : 175;
        std::uint8_t b = border ? 34 : (cx + cy) % 2 == 0 ? 215 : 180;
        // 三个角分别涂成红、绿、蓝，纹理朝向一目了然。
        if (cx == 0 && cy == 0) { r = 230; g = 40; b = 40; }
        if (cx == 7 && cy == 0) { r = 40; g = 200; b = 70; }
        if (cx == 0 && cy == 7) { r = 50; g = 80; b = 220; }
        const std::size_t i = static_cast<std::size_t>(y * kSize + x) * 3;
        uvgrid[i] = r;
        uvgrid[i + 1] = g;
        uvgrid[i + 2] = b;
      }
    }
    write_ppm(texture_dir / "uvgrid.ppm", kSize, kSize, uvgrid);
  }

  // ---- 展示用例：十个新场景 ----
  write_text(scene_dir / "materials.scene", R"(# 材质一览：漫反射、三种粗糙度的金属、玻璃、自发光。
camera 0 1.05 4.6  0 0.55 0  0 1 0  30
ambient 0.05 0.05 0.055
background 130 165 200
sky 1

point_light 2.5 4 3  1 0.97 0.9  40
area_light -1.5 4 -1  3 0 0  0 0 3  9 9 9

material floor diffuse 0.75 0.73 0.7 assets/textures/checker.ppm
material red diffuse 0.75 0.2 0.15 none
material metal0 metal 0.92 0.92 0.94 0.02
material metal1 metal 0.92 0.85 0.6 0.3
material metal2 metal 0.9 0.6 0.4 0.65
material glass dielectric 0.98 0.98 0.98 1.5
material glow emissive 8 8 8

mesh floor 0 0 0  4 1 4  0  assets/meshes/plane.obj
sphere red -2.5 0.45 0  0.45
sphere metal0 -1.5 0.45 0  0.45
sphere metal1 -0.5 0.45 0  0.45
sphere metal2 0.5 0.45 0  0.45
sphere glass 1.5 0.45 0  0.45
sphere glow 2.5 0.45 0  0.45
)");

  write_text(scene_dir / "glass.scene", R"(# 三种折射率的玻璃球，背后放彩色立方体看折射。
camera 0 0.9 3.6  0 0.6 0  0 1 0  32
ambient 0.045 0.045 0.05
background 120 150 185
sky 1

point_light 1.8 3.2 2.4  1 1 1  30
area_light -1 3.5 -0.5  2 0 0  0 0 2  8 8 8

material floor diffuse 0.8 0.78 0.75 assets/textures/tiles.ppm
material glass13 dielectric 0.97 0.97 0.97 1.33
material glass15 dielectric 0.97 0.97 0.97 1.5
material glass18 dielectric 0.97 0.97 0.97 1.8
material red diffuse 0.8 0.15 0.12 none
material green diffuse 0.2 0.7 0.25 none
material blue diffuse 0.25 0.45 0.85 none

mesh floor 0 0 0  3.5 1 3.5  0  assets/meshes/plane.obj
sphere glass13 -1.3 0.5 0.2  0.5
sphere glass15 0 0.5 0.2  0.5
sphere glass18 1.3 0.5 0.2  0.5
mesh red -1.3 0.4 -1.6  0.4 0.4 0.4  20  assets/meshes/cube.obj
mesh green 0 0.4 -1.8  0.4 0.4 0.4  -15  assets/meshes/cube.obj
mesh blue 1.3 0.4 -1.6  0.4 0.4 0.4  30  assets/meshes/cube.obj
)");

  write_text(scene_dir / "roughness.scene", R"(# 金属粗糙度梯度：从镜面到近乎漫反射。
camera 0 0.85 4.2  0 0.5 0  0 1 0  30
ambient 0.04 0.04 0.045
background 135 160 190
sky 1

point_light -2 3.4 2.6  1 0.98 0.92  32
area_light -1.2 3.6 -0.8  2.4 0 0  0 0 2.4  8.5 8.5 8.5

material floor diffuse 0.75 0.73 0.7 assets/textures/checker.ppm
material gold0 metal 0.95 0.78 0.35 0
material gold1 metal 0.95 0.78 0.35 0.25
material gold2 metal 0.95 0.78 0.35 0.5
material gold3 metal 0.95 0.78 0.35 0.75
material gold4 metal 0.95 0.78 0.35 1.0

mesh floor 0 0 0  4 1 4  0  assets/meshes/plane.obj
sphere gold0 -1.8 0.42 0  0.42
sphere gold1 -0.9 0.42 0  0.42
sphere gold2 0 0.42 0  0.42
sphere gold3 0.9 0.42 0  0.42
sphere gold4 1.8 0.42 0  0.42
)");

  write_text(scene_dir / "texture.scene", R"(# 纹理专场：地面瓷砖、渐变墙、UV 网格墙、棋盘球、UV 网格立方体。
camera 0 1.15 3.6  0 0.75 0  0 1 0  36
ambient 0.06 0.06 0.065
background 24 26 34
sky 0

point_light 1.6 2.8 2.2  1 0.97 0.9  26
point_light -2 1.6 1.8  0.6 0.7 1  10
area_light -0.8 3.2 -0.4  1.6 0 0  0 0 1.6  7 7 7

material floor diffuse 0.9 0.9 0.88 assets/textures/tiles.ppm
material rainbow diffuse 1 1 1 assets/textures/gradient.ppm
material grid diffuse 1 1 1 assets/textures/uvgrid.ppm
material checker diffuse 1 1 1 assets/textures/checker.ppm
material clay diffuse 0.7 0.3 0.2 none

mesh floor 0 0 0  2.6 1 2.6  0  assets/meshes/plane.obj
mesh rainbow 0 0 -1.9  1.9 1 1  0  assets/meshes/wall.obj
mesh grid -1.9 0 0  1.9 1 1  90  assets/meshes/wall.obj
mesh checker -0.75 0.5 0.15  0.5 0.5 0.5  0  assets/meshes/sphere.obj
mesh grid 0.75 0.42 0.3  0.42 0.42 0.42  24  assets/meshes/cube.obj
mesh clay 0 0.25 1.15  0.5 0.5 0.5  0  assets/meshes/cone.obj
)");

  write_text(scene_dir / "cornell_pyramid.scene", R"(# 康奈尔盒里的多材质锥体：一个 OBJ 同时带漫反射、金属、玻璃（usemtl 覆盖）。
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
material side_a diffuse 0.85 0.55 0.2 none
material side_b diffuse 0.3 0.55 0.85 none
material side_c metal 0.9 0.85 0.6 0.12
material side_d dielectric 0.95 0.95 0.95 1.5
material base diffuse 0.6 0.6 0.62 none
material gold metal 0.92 0.84 0.62 0.05

mesh white 0 0 0  1 1 1  0  assets/meshes/cornell.obj
mesh base 0.05 0 0.1  0.95 0.95 0.95  25  assets/meshes/pyramid.obj
sphere gold -0.55 0.24 0.5  0.24
)");

  write_text(scene_dir / "emissive.scene", R"(# 暗室霓虹：自发光球和彩色面积光，看颜色溢出到白墙。
camera 0 1 4  0 0.7 0  0 1 0  32
ambient 0.01 0.01 0.012
background 0 0 0
sky 0

point_light 0 3 2  1 1 1  6
area_light -1.6 3 -0.5  1.2 0 0  0 0 1.2  12 3 3
area_light 0.6 3 -0.5  1.2 0 0  0 0 1.2  3 3 12

material floor diffuse 0.6 0.6 0.6 assets/textures/checker.ppm
material red_glow emissive 14 1.5 1.5
material green_glow emissive 2 14 2
material blue_glow emissive 1.5 1.5 14
material white diffuse 0.7 0.7 0.7 none

mesh floor 0 0 0  3 1 3  0  assets/meshes/plane.obj
sphere red_glow -1.2 0.35 0  0.35
sphere green_glow 0 0.35 -0.4  0.35
sphere blue_glow 1.2 0.35 0  0.35
mesh white 0 0.5 -1.5  0.5 0.5 0.5  20  assets/meshes/cube.obj
)");

  write_text(scene_dir / "overlap.scene", R"(# 边界情况：相互穿透的球、嵌套玻璃球、半埋的球、与地板相切的球。
camera 0 1.05 3.8  0 0.5 0  0 1 0  30
ambient 0.05 0.05 0.055
background 135 160 190
sky 1

point_light 2.2 3.4 2.4  1 0.97 0.9  34
area_light -1.2 3.6 -0.8  2.4 0 0  0 0 2.4  9 9 9

material floor diffuse 0.75 0.73 0.7 assets/textures/checker.ppm
material clay diffuse 0.74 0.3 0.18 none
material metal metal 0.9 0.88 0.85 0.15
material glass dielectric 0.97 0.97 0.97 1.5
material glass2 dielectric 0.9 0.95 0.9 1.33

mesh floor 0 0 0  3.5 1 3.5  0  assets/meshes/plane.obj
sphere clay -0.55 0.42 -0.6  0.42
sphere metal -0.05 0.45 -0.75  0.42
sphere glass 1.05 0.5 0.3  0.5
sphere glass2 1.05 0.42 0.3  0.22
sphere metal -1.15 0.12 0.5  0.4
sphere clay 0.35 0.4 1.0  0.4
)");

  write_text(scene_dir / "army.scene", R"(# 球阵：25 个球测试大量图元和 BVH。正中那颗是灯。
camera 0 3.3 6.6  0 0.2 0  0 1 0  36
ambient 0.05 0.05 0.055
background 130 160 195
sky 1

point_light 2.5 4.5 3  1 0.97 0.9  40
point_light -3 2.5 2  0.6 0.72 1  14
area_light -1.5 4.5 -1  3 0 0  0 0 3  9 9 9

material floor diffuse 0.75 0.73 0.7 assets/textures/checker.ppm
material clay diffuse 0.74 0.3 0.18 none
material glaze diffuse 0.28 0.46 0.82 none
material gold metal 0.94 0.76 0.32 0.12
material glass dielectric 0.97 0.97 0.97 1.5
material glow emissive 7 7 7

mesh floor 0 0 0  5 1 5  0  assets/meshes/plane.obj
)");

  write_text(scene_dir / "softshadow.scene", R"(# 一盏大面积灯：软阴影。
camera 2.2 1.5 3.4  0 0.45 0  0 1 0  34
ambient 0.018 0.018 0.022
background 14 16 24
sky 0

point_light 0 3.2 1.2  1 0.95 0.85  9
area_light -1.5 3.4 -1.5  3 0 0  0 0 3  6 5.6 4.8

material floor diffuse 0.85 0.83 0.8 assets/textures/tiles.ppm
material white diffuse 0.72 0.7 0.68 none

mesh floor 0 0 0  3 1 3  0  assets/meshes/plane.obj
mesh white -0.55 0.8 -0.7  0.35 0.8 0.35  15  assets/meshes/cube.obj
mesh white 0.85 0.32 0.15  0.3 0.32 0.3  -10  assets/meshes/cube.obj
sphere white 0.05 0.26 0.95  0.26
)");

  write_text(scene_dir / "stairs.scene", R"(# 楼梯：台阶之间的深层遮挡，圆锥和玻璃球做点缀。
camera 2.7 1.9 3.4  -0.2 0.55 -0.2  0 1 0  34
ambient 0.05 0.05 0.055
background 130 160 195
sky 1

point_light 2 3.6 2.4  1 0.97 0.9  32
area_light -1.2 4 -0.8  2.4 0 0  0 0 2.4  8.5 8.5 8.5

material floor diffuse 0.75 0.73 0.7 assets/textures/checker.ppm
material steps diffuse 0.85 0.83 0.8 assets/textures/tiles.ppm
material red diffuse 0.75 0.2 0.15 none
material gold metal 0.94 0.76 0.32 0.1
material glass dielectric 0.97 0.97 0.97 1.5

mesh floor 0 0 0  3.5 1 3.5  0  assets/meshes/plane.obj
mesh steps -0.4 0 -0.3  1 1 1  0  assets/meshes/stairs.obj
mesh red -1.6 0 0.9  0.7 0.7 0.7  0  assets/meshes/cone.obj
sphere gold -0.6 0.26 1.1  0.26
sphere glass 0.52 1.19 -0.3  0.23
)");

  write_human_mesh(mesh_dir / "human.obj");

  write_text(scene_dir / "human.scene", R"(# 木偶人：程序化生成的椭球关节人偶，一大一小。
camera 0.5 1.3 3.8  -0.1 0.8 0  0 1 0  30
ambient 0.05 0.05 0.055
background 130 160 195
sky 1

point_light 2.2 3.4 2.6  1 0.96 0.88  34
area_light -1.2 3.6 -0.8  2.4 0 0  0 0 2.4  9 9 9

material floor diffuse 0.85 0.83 0.8 assets/textures/tiles.ppm
material skin diffuse 0.85 0.6 0.45 none
material shirt diffuse 0.3 0.5 0.8 none
material pants diffuse 0.25 0.28 0.35 none
material shoes diffuse 0.2 0.15 0.12 none
material eye diffuse 0.05 0.05 0.06 none
material gold metal 0.94 0.76 0.32 0.1
material glass dielectric 0.97 0.97 0.97 1.5

mesh floor 0 0 0  3.5 1 3.5  0  assets/meshes/plane.obj
mesh skin -0.45 0 0  1 1 1  12  assets/meshes/human.obj
mesh skin 0.75 0 -0.7  0.62 0.62 0.62  -18  assets/meshes/human.obj
sphere gold -1.35 0.28 0.8  0.28
sphere glass 1.35 0.32 0.5  0.32
)");

  // army.scene 的 25 个球用循环追加，材质按位置轮换。
  {
    std::ostringstream army;
    std::ifstream in(scene_dir / "army.scene");
    army << in.rdbuf();
    const char* mats[4] = {"clay", "glaze", "gold", "glass"};
    for (int i = 0; i < 25; ++i) {
      if (i == 12) {
        army << "sphere glow " << 0.0f << ' ' << 0.34f << ' ' << 0.0f << "  0.34\n";
        continue;
      }
      const float x = static_cast<float>(i % 5 - 2) * 0.85f;
      const float z = static_cast<float>(i / 5 - 2) * 0.85f;
      army << "sphere " << mats[i % 4] << ' ' << x << " 0.34 " << z << "  0.34\n";
    }
    write_text(scene_dir / "army.scene", army.str());
  }

  std::cout << "素材已写到 " << (root / "assets").string() << '\n';
  return 0;
}
