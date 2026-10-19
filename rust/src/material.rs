use crate::vec::Vec3;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MaterialType {
    Diffuse,
    Metal,
    Dielectric,
    Emissive,
}

#[derive(Clone, Debug)]
pub struct Material {
    pub name: String,
    pub kind: MaterialType,
    pub albedo: Vec3,
    pub roughness: f32,
    pub ior: f32,
    pub emission: Vec3,
    pub texture: i32,
}

impl Default for Material {
    fn default() -> Self {
        Self {
            name: String::new(),
            kind: MaterialType::Diffuse,
            albedo: Vec3::new(0.8, 0.8, 0.8),
            roughness: 0.4,
            ior: 1.5,
            emission: Vec3::default(),
            texture: -1,
        }
    }
}
