#pragma once

#include "camera.hpp"
#include "color.hpp"
#include "triangle.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace gfx {

struct Varying {
  float x = 0;
  float y = 0;
  float z = 0;
  float inv_w = 0;
  Vec3 world;
  Vec3 normal;
  Vec2 uv;
  Vec3 color{1, 1, 1};
};

struct ClipVert {
  Vec4 clip;
  Vec3 world;
  Vec3 normal;
  Vec2 uv;
  Vec3 color{1, 1, 1};
};

struct DepthBuffer {
  int width = 0;
  int height = 0;
  std::vector<float> z;

  DepthBuffer() = default;
  DepthBuffer(int w, int h) { reset(w, h); }

  void reset(int w, int h) {
    width = w;
    height = h;
    z.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
             std::numeric_limits<float>::infinity());
  }

  void clear() { std::fill(z.begin(), z.end(), std::numeric_limits<float>::infinity()); }

  bool pass(int x, int y, float depth) {
    if (!std::isfinite(depth)) return false;
    float& slot = z[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)];
    if (!(depth < slot)) return false;
    slot = depth;
    return true;
  }
};

inline ClipVert lerp_clip(const ClipVert& a, const ClipVert& b, float t) {
  ClipVert out;
  out.clip = a.clip + (b.clip - a.clip) * t;
  out.world = lerp(a.world, b.world, t);
  out.normal = lerp(a.normal, b.normal, t);
  out.uv = lerp(a.uv, b.uv, t);
  out.color = lerp(a.color, b.color, t);
  return out;
}

template <class Dist>
void clip_against(std::vector<ClipVert>& polygon, Dist dist) {
  if (polygon.empty()) return;
  std::vector<ClipVert> out;
  out.reserve(polygon.size() + 1);
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const ClipVert& current = polygon[i];
    const ClipVert& previous = polygon[(i + polygon.size() - 1) % polygon.size()];
    const float current_d = dist(current);
    const float previous_d = dist(previous);
    const bool current_in = current_d >= 0.0f;
    const bool previous_in = previous_d >= 0.0f;
    if (current_in != previous_in) {
      const float denom = previous_d - current_d;
      if (std::fabs(denom) > 1e-8f) out.push_back(lerp_clip(previous, current, previous_d / denom));
    }
    if (current_in) out.push_back(current);
  }
  polygon.swap(out);
}

// 在裁剪空间用六个平面把三角形裁成凸多边形。平面是 x±w、y±w、z±w。
inline void clip_frustum(std::vector<ClipVert>& polygon) {
  clip_against(polygon, [](const ClipVert& v) { return v.clip.x + v.clip.w; });
  clip_against(polygon, [](const ClipVert& v) { return v.clip.w - v.clip.x; });
  clip_against(polygon, [](const ClipVert& v) { return v.clip.y + v.clip.w; });
  clip_against(polygon, [](const ClipVert& v) { return v.clip.w - v.clip.y; });
  clip_against(polygon, [](const ClipVert& v) { return v.clip.z + v.clip.w; });
  clip_against(polygon, [](const ClipVert& v) { return v.clip.w - v.clip.z; });
}

inline bool to_varying(const ClipVert& vertex, int width, int height, Varying& out) {
  if (!(vertex.clip.w > 1e-6f) || !std::isfinite(vertex.clip.w)) return false;
  const float inv_w = 1.0f / vertex.clip.w;
  const float ndc_x = vertex.clip.x * inv_w;
  const float ndc_y = vertex.clip.y * inv_w;
  const float ndc_z = vertex.clip.z * inv_w;
  out.x = (ndc_x + 1.0f) * 0.5f * static_cast<float>(width);
  out.y = (1.0f - (ndc_y + 1.0f) * 0.5f) * static_cast<float>(height);
  out.z = ndc_z * 0.5f + 0.5f;
  out.inv_w = inv_w;
  out.world = vertex.world;
  out.normal = vertex.normal;
  out.uv = vertex.uv;
  out.color = vertex.color;
  return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

template <class T>
T perspective_interpolate(const T& a, const T& b, const T& c, float w0, float w1, float w2, float iw0,
                          float iw1, float iw2) {
  const float inv_w = w0 * iw0 + w1 * iw1 + w2 * iw2;
  return (a * (w0 * iw0) + b * (w1 * iw1) + c * (w2 * iw2)) * (1.0f / inv_w);
}

template <class Shader>
void rasterize_triangle(Image& image, DepthBuffer& depth, Varying v0, Varying v1, Varying v2, Shader&& shader) {
  if (edge({v0.x, v0.y}, {v1.x, v1.y}, {v2.x, v2.y}) < 0.0f) std::swap(v1, v2);
  const Vec2 a{v0.x, v0.y};
  const Vec2 b{v1.x, v1.y};
  const Vec2 c{v2.x, v2.y};
  const float area = edge(a, b, c);
  if (!(area > 1e-6f)) return;

  const int x0 = std::clamp(static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))), 0, image.width() - 1);
  const int x1 = std::clamp(static_cast<int>(std::floor(std::max({a.x, b.x, c.x}))), 0, image.width() - 1);
  const int y0 = std::clamp(static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))), 0, image.height() - 1);
  const int y1 = std::clamp(static_cast<int>(std::floor(std::max({a.y, b.y, c.y}))), 0, image.height() - 1);
  const bool top0 = is_top_left(b, c);
  const bool top1 = is_top_left(c, a);
  const bool top2 = is_top_left(a, b);
  const auto inside = [](float w, bool top_left) { return w > 0.0f || (w == 0.0f && top_left); };

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const Vec2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
      const float e0 = edge(b, c, p);
      const float e1 = edge(c, a, p);
      const float e2 = edge(a, b, p);
      if (!inside(e0, top0) || !inside(e1, top1) || !inside(e2, top2)) continue;
      const float w0 = e0 / area;
      const float w1 = e1 / area;
      const float w2 = e2 / area;
      const float z = w0 * v0.z + w1 * v1.z + w2 * v2.z;
      if (!depth.pass(x, y, z)) continue;

      const float denom = w0 * v0.inv_w + w1 * v1.inv_w + w2 * v2.inv_w;
      if (!(std::fabs(denom) > 1e-12f)) continue;
      Varying frag;
      frag.x = p.x;
      frag.y = p.y;
      frag.z = z;
      frag.inv_w = denom;
      frag.world = perspective_interpolate(v0.world, v1.world, v2.world, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w);
      frag.normal = perspective_interpolate(v0.normal, v1.normal, v2.normal, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w);
      const float normal_len = length(frag.normal);
      if (normal_len > 1e-8f) frag.normal = frag.normal / normal_len;
      frag.uv = perspective_interpolate(v0.uv, v1.uv, v2.uv, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w);
      frag.color = perspective_interpolate(v0.color, v1.color, v2.color, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w);
      write_linear(image, x, y, shader(frag));
    }
  }
}

template <class Shader>
void draw_clipped_triangle(Image& image, DepthBuffer& depth, ClipVert a, ClipVert b, ClipVert c, Shader&& shader) {
  std::vector<ClipVert> polygon{a, b, c};
  clip_frustum(polygon);
  if (polygon.size() < 3) return;

  std::vector<Varying> projected(polygon.size());
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    if (!to_varying(polygon[i], image.width(), image.height(), projected[i])) return;
  }
  for (std::size_t i = 1; i + 1 < projected.size(); ++i) {
    rasterize_triangle(image, depth, projected[0], projected[i], projected[i + 1], shader);
  }
}

}  // namespace gfx
