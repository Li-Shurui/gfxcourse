#pragma once

#include "world.hpp"

#include <algorithm>
#include <vector>

namespace gfx {

struct Aabb {
  Vec3 mn{1e30f, 1e30f, 1e30f};
  Vec3 mx{-1e30f, -1e30f, -1e30f};

  void add(Vec3 point) {
    mn = min(mn, point);
    mx = max(mx, point);
  }

  void add(const Aabb& other) {
    mn = min(mn, other.mn);
    mx = max(mx, other.mx);
  }

  int longest_axis() const {
    const Vec3 size = mx - mn;
    if (size.x > size.y && size.x > size.z) return 0;
    return size.y > size.z ? 1 : 2;
  }

  bool hit(const Ray& ray, float tmin, float tmax) const {
    for (int axis = 0; axis < 3; ++axis) {
      const float direction = ray.direction[axis];
      if (std::fabs(direction) < 1e-8f) {
        if (ray.origin[axis] < mn[axis] || ray.origin[axis] > mx[axis]) return false;
        continue;
      }
      const float inv = 1.0f / direction;
      float t0 = (mn[axis] - ray.origin[axis]) * inv;
      float t1 = (mx[axis] - ray.origin[axis]) * inv;
      if (t0 > t1) std::swap(t0, t1);
      tmin = std::max(tmin, t0);
      tmax = std::min(tmax, t1);
      if (tmax < tmin) return false;
    }
    return true;
  }
};

struct BvhNode {
  Aabb box;
  int left = -1;
  int right = -1;
  int start = 0;
  int count = 0;
};

struct Bvh {
  std::vector<int> order;
  std::vector<BvhNode> nodes;
  int root = -1;

  static Bvh build(const std::vector<Tri>& triangles) {
    Bvh tree;
    tree.order.resize(triangles.size());
    for (int i = 0; i < static_cast<int>(triangles.size()); ++i) tree.order[static_cast<std::size_t>(i)] = i;
    if (!triangles.empty()) tree.root = tree.build_range(triangles, 0, static_cast<int>(triangles.size()));
    return tree;
  }

  bool closest(const std::vector<Tri>& triangles, const Ray& ray, float tmin, float tmax, Hit& hit) const {
    if (root < 0) return false;
    std::vector<int> stack;
    stack.push_back(root);
    bool found = false;
    while (!stack.empty()) {
      const BvhNode& node = nodes[static_cast<std::size_t>(stack.back())];
      stack.pop_back();
      if (!node.box.hit(ray, tmin, tmax)) continue;
      if (node.count > 0) {
        for (int i = 0; i < node.count; ++i) {
          const Tri& tri = triangles[static_cast<std::size_t>(order[static_cast<std::size_t>(node.start + i)])];
          if (intersect_triangle(ray, tri, tmin, tmax, hit)) {
            found = true;
            tmax = hit.t;
          }
        }
      } else {
        stack.push_back(node.left);
        stack.push_back(node.right);
      }
    }
    return found;
  }

 private:
  static Vec3 centroid(const Tri& tri) { return (tri.p[0] + tri.p[1] + tri.p[2]) * (1.0f / 3.0f); }

  static Aabb bounds_of(const Tri& tri) {
    Aabb box;
    box.add(tri.p[0]);
    box.add(tri.p[1]);
    box.add(tri.p[2]);
    box.mn = box.mn - gray(1e-4f);
    box.mx = box.mx + gray(1e-4f);
    return box;
  }

  int build_range(const std::vector<Tri>& triangles, int start, int count) {
    Aabb bounds;
    Aabb centroids;
    for (int i = 0; i < count; ++i) {
      const Tri& tri = triangles[static_cast<std::size_t>(order[static_cast<std::size_t>(start + i)])];
      const Aabb box = bounds_of(tri);
      bounds.add(box);
      centroids.add(centroid(tri));
    }
    const int node_index = static_cast<int>(nodes.size());
    nodes.push_back({});
    const Vec3 extent = centroids.mx - centroids.mn;
    const bool tiny = extent.x < 1e-5f && extent.y < 1e-5f && extent.z < 1e-5f;
    if (count <= 4 || tiny) {
      nodes[static_cast<std::size_t>(node_index)].box = bounds;
      nodes[static_cast<std::size_t>(node_index)].start = start;
      nodes[static_cast<std::size_t>(node_index)].count = count;
      return node_index;
    }

    const int axis = centroids.longest_axis();
    const float mid = 0.5f * (centroids.mn[axis] + centroids.mx[axis]);
    int split = start;
    for (int i = start; i < start + count; ++i) {
      const float value = centroid(triangles[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])])[axis];
      if (value < mid) std::swap(order[static_cast<std::size_t>(i)], order[static_cast<std::size_t>(split++)]);
    }
    int left_count = split - start;
    if (left_count == 0 || left_count == count) {
      left_count = count / 2;
      std::nth_element(order.begin() + start, order.begin() + start + left_count, order.begin() + start + count,
                       [&](int a, int b) {
                         return centroid(triangles[static_cast<std::size_t>(a)])[axis] <
                                centroid(triangles[static_cast<std::size_t>(b)])[axis];
                       });
    }
    const int left = build_range(triangles, start, left_count);
    const int right = build_range(triangles, start + left_count, count - left_count);
    nodes[static_cast<std::size_t>(node_index)].left = left;
    nodes[static_cast<std::size_t>(node_index)].right = right;
    nodes[static_cast<std::size_t>(node_index)].box = bounds;
    return node_index;
  }
};

inline bool hit_scene(const RWorld& world, const Bvh& bvh, const Ray& ray, float tmin, float tmax, Hit& hit) {
  bool found = false;
  for (const Sphere& sphere : world.spheres) {
    if (sphere.intersect(ray, tmin, tmax, hit)) {
      found = true;
      tmax = hit.t;
    }
  }
  Hit triangle_hit;
  if (bvh.closest(world.triangles, ray, tmin, tmax, triangle_hit)) {
    hit = triangle_hit;
    found = true;
  }
  return found;
}

}  // namespace gfx
