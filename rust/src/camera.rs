use crate::mat::Mat4;
use crate::vec::*;

#[derive(Clone, Copy, Debug)]
pub struct Camera {
    pub eye: Vec3,
    pub target: Vec3,
    pub up: Vec3,
    pub fov_y_degrees: f32,
    pub z_near: f32,
    pub z_far: f32,
}

impl Default for Camera {
    fn default() -> Self {
        Self {
            eye: Vec3::new(0.0, 0.0, 3.0),
            target: Vec3::default(),
            up: Vec3::new(0.0, 1.0, 0.0),
            fov_y_degrees: 45.0,
            z_near: 0.1,
            z_far: 100.0,
        }
    }
}

pub fn view_matrix(camera: &Camera) -> Mat4 {
    let forward = normalize3(camera.target - camera.eye);
    let right = normalize3(cross(forward, camera.up));
    let up = cross(right, forward);
    let mut m = Mat4::identity();
    m.set(0, 0, right.x);
    m.set(0, 1, right.y);
    m.set(0, 2, right.z);
    m.set(0, 3, -dot3(right, camera.eye));
    m.set(1, 0, up.x);
    m.set(1, 1, up.y);
    m.set(1, 2, up.z);
    m.set(1, 3, -dot3(up, camera.eye));
    m.set(2, 0, -forward.x);
    m.set(2, 1, -forward.y);
    m.set(2, 2, -forward.z);
    m.set(2, 3, dot3(forward, camera.eye));
    m
}

pub fn perspective(fov_y_degrees: f32, aspect: f32, z_near: f32, z_far: f32) -> Mat4 {
    let f = 1.0 / (radians(fov_y_degrees) * 0.5).tan();
    let mut m = Mat4::zero();
    m.set(0, 0, f / aspect);
    m.set(1, 1, f);
    m.set(2, 2, (z_far + z_near) / (z_near - z_far));
    m.set(3, 2, -1.0);
    m.set(2, 3, (2.0 * z_far * z_near) / (z_near - z_far));
    m
}

pub fn view_projection(camera: &Camera, aspect: f32) -> Mat4 {
    perspective(camera.fov_y_degrees, aspect, camera.z_near, camera.z_far) * view_matrix(camera)
}

pub fn clip_line_near(a: &mut Vec4, b: &mut Vec4) -> bool {
    let da = a.z + a.w;
    let db = b.z + b.w;
    if da < 0.0 && db < 0.0 {
        return false;
    }
    if da >= 0.0 && db >= 0.0 {
        return true;
    }
    let denom = da - db;
    if denom.abs() < 1e-8 {
        return false;
    }
    let point = *a + (*b - *a) * (da / denom);
    if da < 0.0 {
        *a = point;
    } else {
        *b = point;
    }
    true
}

pub fn project_to_screen(clip: Vec4, width: usize, height: usize) -> Option<(f32, f32)> {
    if !(clip.w > 1e-6) || !clip.w.is_finite() {
        return None;
    }
    let inv_w = 1.0 / clip.w;
    let ndc_x = clip.x * inv_w;
    let ndc_y = clip.y * inv_w;
    let x = (ndc_x + 1.0) * 0.5 * width as f32;
    let y = (1.0 - (ndc_y + 1.0) * 0.5) * height as f32;
    if x.is_finite() && y.is_finite() {
        Some((x, y))
    } else {
        None
    }
}
