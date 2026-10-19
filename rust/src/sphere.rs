use crate::ray::{Hit, Ray};
use crate::vec::{dot3, length_squared3, Vec2, Vec3, K_PI};

#[derive(Clone, Copy, Debug)]
pub struct Sphere {
    pub center: Vec3,
    pub radius: f32,
    pub material: usize,
}

impl Sphere {
    pub fn intersect(&self, ray: &Ray, tmin: f32, tmax: f32, hit: &mut Hit) -> bool {
        let oc = ray.origin - self.center;
        let a = length_squared3(ray.direction);
        let half_b = dot3(oc, ray.direction);
        let c = length_squared3(oc) - self.radius * self.radius;
        let discriminant = half_b * half_b - a * c;
        if discriminant < 0.0 {
            return false;
        }
        let root = discriminant.sqrt();
        let mut t = (-half_b - root) / a;
        if t < tmin || t > tmax {
            t = (-half_b + root) / a;
            if t < tmin || t > tmax {
                return false;
            }
        }
        hit.t = t;
        hit.position = ray.at(t);
        let outward = (hit.position - self.center) / self.radius;
        hit.geometric = outward;
        hit.front = dot3(ray.direction, outward) < 0.0;
        hit.normal = if hit.front { outward } else { -outward };
        let phi = (-outward.z).atan2(outward.x) + K_PI;
        let theta = outward.y.clamp(-1.0, 1.0).acos();
        hit.uv = Vec2::new(phi / (2.0 * K_PI), theta / K_PI);
        hit.material = self.material as i32;
        true
    }
}
