use crate::ray::{Hit, Ray};
use crate::vec::{cross, dot3, length_squared3, normalize3, Vec2, Vec3};

#[derive(Clone, Copy, Debug)]
pub struct Tri {
    pub p: [Vec3; 3],
    pub n: [Vec3; 3],
    pub uv: [Vec2; 3],
    pub material: usize,
}

pub fn intersect_triangle(ray: &Ray, tri: &Tri, tmin: f32, tmax: f32, hit: &mut Hit) -> bool {
    const EPS: f32 = 1e-8;
    let e1 = tri.p[1] - tri.p[0];
    let e2 = tri.p[2] - tri.p[0];
    let pvec = cross(ray.direction, e2);
    let det = dot3(e1, pvec);
    if det.abs() < EPS {
        return false;
    }
    let inv_det = 1.0 / det;
    let tvec = ray.origin - tri.p[0];
    let u = dot3(tvec, pvec) * inv_det;
    if u < 0.0 || u > 1.0 {
        return false;
    }
    let qvec = cross(tvec, e1);
    let v = dot3(ray.direction, qvec) * inv_det;
    if v < 0.0 || u + v > 1.0 {
        return false;
    }
    let t = dot3(e2, qvec) * inv_det;
    if t < tmin || t > tmax {
        return false;
    }

    let w = 1.0 - u - v;
    let mut outward = cross(e1, e2);
    if length_squared3(outward) < EPS {
        return false;
    }
    outward = normalize3(outward);
    let mut shading = tri.n[0] * w + tri.n[1] * u + tri.n[2] * v;
    if length_squared3(shading) < EPS {
        shading = outward;
    } else {
        shading = normalize3(shading);
    }

    hit.t = t;
    hit.position = ray.at(t);
    hit.geometric = outward;
    hit.front = dot3(ray.direction, outward) < 0.0;
    let face = if hit.front { outward } else { -outward };
    if dot3(shading, face) < 0.0 {
        shading = -shading;
    }
    hit.normal = shading;
    hit.uv = tri.uv[0] * w + tri.uv[1] * u + tri.uv[2] * v;
    hit.material = tri.material as i32;
    true
}
