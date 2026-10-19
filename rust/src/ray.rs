use crate::camera::Camera;
use crate::vec::{lerp3, normalize3, Vec2, Vec3};

#[derive(Clone, Copy, Debug)]
pub struct Ray {
    pub origin: Vec3,
    pub direction: Vec3,
}

impl Ray {
    pub fn at(&self, t: f32) -> Vec3 {
        self.origin + self.direction * t
    }
}

#[derive(Clone, Copy, Debug)]
pub struct Hit {
    pub t: f32,
    pub position: Vec3,
    pub normal: Vec3,
    pub geometric: Vec3,
    pub uv: Vec2,
    pub material: i32,
    pub front: bool,
}

impl Default for Hit {
    fn default() -> Self {
        Self {
            t: 0.0,
            position: Vec3::default(),
            normal: Vec3::default(),
            geometric: Vec3::default(),
            uv: Vec2::default(),
            material: -1,
            front: true,
        }
    }
}

pub fn sky_color(ray: &Ray) -> Vec3 {
    let t = 0.5 * (normalize3(ray.direction).y + 1.0);
    lerp3(Vec3::new(1.0, 1.0, 1.0), Vec3::new(0.45, 0.65, 0.95), t)
}

pub fn camera_ray(camera: &Camera, film_x: f32, film_y: f32, width: usize, height: usize) -> Ray {
    let aspect = width as f32 / height as f32;
    let half_height = (crate::vec::radians(camera.fov_y_degrees) * 0.5).tan();
    let half_width = aspect * half_height;
    let forward = normalize3(camera.target - camera.eye);
    let right = normalize3(crate::vec::cross(forward, camera.up));
    let up = crate::vec::cross(right, forward);
    let sx = (2.0 * (film_x / width as f32) - 1.0) * half_width;
    let sy = (1.0 - 2.0 * (film_y / height as f32)) * half_height;
    Ray {
        origin: camera.eye,
        direction: normalize3(forward + right * sx + up * sy),
    }
}
