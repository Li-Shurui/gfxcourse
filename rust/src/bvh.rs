use crate::ray::{Hit, Ray};
use crate::trimesh::{intersect_triangle, Tri};
use crate::vec::{gray, max, min, Vec3};
use crate::world::RWorld;

#[derive(Clone, Copy, Debug)]
pub struct Aabb {
    pub mn: Vec3,
    pub mx: Vec3,
}

impl Default for Aabb {
    fn default() -> Self {
        Self {
            mn: Vec3::new(1e30, 1e30, 1e30),
            mx: Vec3::new(-1e30, -1e30, -1e30),
        }
    }
}

impl Aabb {
    pub fn add(&mut self, point: Vec3) {
        self.mn = min(self.mn, point);
        self.mx = max(self.mx, point);
    }

    pub fn add_box(&mut self, other: &Aabb) {
        self.mn = min(self.mn, other.mn);
        self.mx = max(self.mx, other.mx);
    }

    pub fn longest_axis(&self) -> usize {
        let size = self.mx - self.mn;
        if size.x > size.y && size.x > size.z {
            0
        } else if size.y > size.z {
            1
        } else {
            2
        }
    }

    pub fn hit(&self, ray: &Ray, mut tmin: f32, mut tmax: f32) -> bool {
        for axis in 0..3 {
            let direction = ray.direction[axis];
            if direction.abs() < 1e-8 {
                if ray.origin[axis] < self.mn[axis] || ray.origin[axis] > self.mx[axis] {
                    return false;
                }
                continue;
            }
            let inv = 1.0 / direction;
            let mut t0 = (self.mn[axis] - ray.origin[axis]) * inv;
            let mut t1 = (self.mx[axis] - ray.origin[axis]) * inv;
            if t0 > t1 {
                std::mem::swap(&mut t0, &mut t1);
            }
            tmin = tmin.max(t0);
            tmax = tmax.min(t1);
            if tmax < tmin {
                return false;
            }
        }
        true
    }
}

#[derive(Clone, Debug)]
pub struct BvhNode {
    pub box_: Aabb,
    pub left: i32,
    pub right: i32,
    pub start: usize,
    pub count: usize,
}

impl Default for BvhNode {
    fn default() -> Self {
        Self {
            box_: Aabb::default(),
            left: -1,
            right: -1,
            start: 0,
            count: 0,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Bvh {
    pub order: Vec<usize>,
    pub nodes: Vec<BvhNode>,
    pub root: i32,
}

impl Bvh {
    pub fn build(triangles: &[Tri]) -> Self {
        let mut tree = Self {
            order: (0..triangles.len()).collect(),
            nodes: Vec::new(),
            root: -1,
        };
        if !triangles.is_empty() {
            tree.root = tree.build_range(triangles, 0, triangles.len());
        }
        tree
    }

    fn centroid(tri: &Tri) -> Vec3 {
        (tri.p[0] + tri.p[1] + tri.p[2]) * (1.0 / 3.0)
    }

    fn bounds_of(tri: &Tri) -> Aabb {
        let mut box_ = Aabb::default();
        box_.add(tri.p[0]);
        box_.add(tri.p[1]);
        box_.add(tri.p[2]);
        box_.mn = box_.mn - gray(1e-4);
        box_.mx = box_.mx + gray(1e-4);
        box_
    }

    fn build_range(&mut self, triangles: &[Tri], start: usize, count: usize) -> i32 {
        let mut bounds = Aabb::default();
        let mut centroids = Aabb::default();
        for i in 0..count {
            let tri = &triangles[self.order[start + i]];
            let box_ = Self::bounds_of(tri);
            bounds.add_box(&box_);
            centroids.add(Self::centroid(tri));
        }
        let node_index = self.nodes.len();
        self.nodes.push(BvhNode::default());
        let extent = centroids.mx - centroids.mn;
        let tiny = extent.x < 1e-5 && extent.y < 1e-5 && extent.z < 1e-5;
        if count <= 4 || tiny {
            self.nodes[node_index].box_ = bounds;
            self.nodes[node_index].start = start;
            self.nodes[node_index].count = count;
            return node_index as i32;
        }

        let axis = centroids.longest_axis();
        let mid = 0.5 * (centroids.mn[axis] + centroids.mx[axis]);
        let mut split = start;
        for i in start..start + count {
            let value = Self::centroid(&triangles[self.order[i]])[axis];
            if value < mid {
                self.order.swap(i, split);
                split += 1;
            }
        }
        let mut left_count = split - start;
        if left_count == 0 || left_count == count {
            left_count = count / 2;
            self.order[start..start + count].select_nth_unstable_by(left_count, |&a, &b| {
                Self::centroid(&triangles[a])[axis]
                    .partial_cmp(&Self::centroid(&triangles[b])[axis])
                    .unwrap()
            });
        }
        let left = self.build_range(triangles, start, left_count);
        let right = self.build_range(triangles, start + left_count, count - left_count);
        self.nodes[node_index].left = left;
        self.nodes[node_index].right = right;
        self.nodes[node_index].box_ = bounds;
        node_index as i32
    }

    pub fn closest(&self, triangles: &[Tri], ray: &Ray, tmin: f32, tmax: f32, hit: &mut Hit) -> bool {
        if self.root < 0 {
            return false;
        }
        let mut stack = vec![self.root];
        let mut found = false;
        let mut tmax = tmax;
        while let Some(node_index) = stack.pop() {
            let node = &self.nodes[node_index as usize];
            if !node.box_.hit(ray, tmin, tmax) {
                continue;
            }
            if node.count > 0 {
                for i in 0..node.count {
                    let tri = &triangles[self.order[node.start + i]];
                    if intersect_triangle(ray, tri, tmin, tmax, hit) {
                        found = true;
                        tmax = hit.t;
                    }
                }
            } else {
                stack.push(node.left);
                stack.push(node.right);
            }
        }
        found
    }
}

pub fn hit_scene(world: &RWorld, bvh: &Bvh, ray: &Ray, tmin: f32, tmax: f32, hit: &mut Hit) -> bool {
    let mut found = false;
    let mut tmax = tmax;
    for sphere in &world.spheres {
        if sphere.intersect(ray, tmin, tmax, hit) {
            found = true;
            tmax = hit.t;
        }
    }
    let mut triangle_hit = Hit::default();
    if bvh.closest(&world.triangles, ray, tmin, tmax, &mut triangle_hit) {
        *hit = triangle_hit;
        found = true;
    }
    found
}
