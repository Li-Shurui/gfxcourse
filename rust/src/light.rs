use crate::vec::{dot3, length_squared3, normalize3, Vec3};

#[derive(Clone, Copy, Debug)]
pub struct PointLight {
    pub position: Vec3,
    pub color: Vec3,
    pub intensity: f32,
}

impl Default for PointLight {
    fn default() -> Self {
        Self {
            position: Vec3::new(0.0, 2.0, 2.0),
            color: Vec3::new(1.0, 1.0, 1.0),
            intensity: 1.0,
        }
    }
}

pub fn blinn_phong(
    position: Vec3,
    normal: Vec3,
    view_position: Vec3,
    diffuse: Vec3,
    specular: Vec3,
    shininess: f32,
    ambient: Vec3,
    lights: &[PointLight],
) -> Vec3 {
    let len = crate::vec::length3(normal);
    if len < 1e-8 {
        return diffuse * ambient;
    }
    let n = normal / len;
    let view = normalize3(view_position - position);
    let mut color = diffuse * ambient;
    for light in lights {
        let delta = light.position - position;
        let dist2 = length_squared3(delta).max(1e-4);
        let l = delta / dist2.sqrt();
        let ndotl = dot3(n, l).max(0.0);
        let radiance = light.color * (light.intensity / dist2);
        color += diffuse * radiance * ndotl;
        if ndotl > 0.0 && shininess > 0.0 {
            let half = normalize3(l + view);
            let spec = dot3(n, half).max(0.0).powf(shininess);
            color += specular * radiance * spec;
        }
    }
    color
}
