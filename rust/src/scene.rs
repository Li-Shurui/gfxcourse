use crate::camera::Camera;
use crate::image::Color;
use crate::light::PointLight;
use crate::material::{Material, MaterialType};
use crate::mesh::{load_obj, Mesh};
use crate::paths::resolve_asset;
use crate::texture::Texture;
use crate::vec::{gray, Vec2, Vec3};
use std::collections::HashMap;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

#[derive(Clone, Debug)]
pub struct Transform {
    pub translation: Vec3,
    pub scale: Vec3,
    pub rotate_y_degrees: f32,
}

impl Default for Transform {
    fn default() -> Self {
        Self {
            translation: Vec3::default(),
            scale: Vec3::new(1.0, 1.0, 1.0),
            rotate_y_degrees: 0.0,
        }
    }
}

impl Transform {
    pub fn matrix(&self) -> crate::mat::Mat4 {
        crate::mat::Mat4::translation(self.translation)
            * crate::mat::Mat4::rotation(Vec3::new(0.0, 1.0, 0.0), crate::vec::radians(self.rotate_y_degrees))
            * crate::mat::Mat4::scale(self.scale)
    }
}

#[derive(Clone, Debug)]
pub struct MeshInstance {
    pub mesh: Mesh,
    pub transform: Transform,
    pub material: usize,
}

#[derive(Clone, Debug)]
pub struct SphereInstance {
    pub center: Vec3,
    pub radius: f32,
    pub material: usize,
}

#[derive(Clone, Debug)]
pub struct AreaLight {
    pub corner: Vec3,
    pub edge_u: Vec3,
    pub edge_v: Vec3,
    pub emission: Vec3,
}

impl AreaLight {
    pub fn normal(&self) -> Vec3 {
        crate::vec::normalize3(crate::vec::cross(self.edge_u, self.edge_v))
    }

    pub fn area(&self) -> f32 {
        crate::vec::length3(crate::vec::cross(self.edge_u, self.edge_v))
    }

    pub fn center(&self) -> Vec3 {
        self.corner + (self.edge_u + self.edge_v) * 0.5
    }

    pub fn sample(&self, u: f32, v: f32) -> Vec3 {
        self.corner + self.edge_u * u + self.edge_v * v
    }
}

#[derive(Clone, Debug)]
pub struct Scene {
    pub camera: Camera,
    pub ambient: Vec3,
    pub background: Color,
    pub sky: bool,
    pub materials: Vec<Material>,
    pub textures: Vec<Texture>,
    pub meshes: Vec<MeshInstance>,
    pub spheres: Vec<SphereInstance>,
    pub point_lights: Vec<PointLight>,
    pub area_lights: Vec<AreaLight>,
    pub material_of: HashMap<String, usize>,
}

impl Default for Scene {
    fn default() -> Self {
        Self {
            camera: Camera::default(),
            ambient: gray(0.03),
            background: Color::new(16, 18, 24),
            sky: false,
            materials: Vec::new(),
            textures: Vec::new(),
            meshes: Vec::new(),
            spheres: Vec::new(),
            point_lights: Vec::new(),
            area_lights: Vec::new(),
            material_of: HashMap::new(),
        }
    }
}

impl Scene {
    pub fn material(&self, index: usize) -> &Material {
        &self.materials[index]
    }
}

pub fn load_scene(path: &Path) -> Result<Scene, String> {
    let file = File::open(path).map_err(|e| format!("无法打开场景: {}: {}", path.display(), e))?;
    let reader = BufReader::new(file);

    let mut tokens = Vec::new();
    for line in reader.lines() {
        let line = line.map_err(|e| format!("读取场景失败: {}", e))?;
        let line = line.trim_end_matches('\r');
        let line = if let Some(comment) = line.find('#') {
            &line[..comment]
        } else {
            line
        };
        tokens.extend(line.split_whitespace().map(|s| s.to_string()));
    }

    struct Cursor {
        tokens: Vec<String>,
        index: usize,
        error: String,
    }

    impl Cursor {
        fn need(&mut self, count: usize) -> bool {
            if !self.error.is_empty() {
                return false;
            }
            if self.index + count <= self.tokens.len() {
                return true;
            }
            self.error = format!(
                "场景参数不足，停在: {}",
                if self.index < self.tokens.len() {
                    &self.tokens[self.index]
                } else {
                    "文件结尾"
                }
            );
            false
        }

        fn take(&mut self) -> &str {
            let token = &self.tokens[self.index];
            self.index += 1;
            token
        }

        fn number(&mut self) -> f32 {
            if !self.need(1) {
                return 0.0;
            }
            let token = self.take().to_string();
            token.parse().unwrap_or_else(|_| {
                self.error = format!("无法解析数字: {}", token);
                0.0
            })
        }

        fn vec3(&mut self) -> Vec3 {
            Vec3::new(self.number(), self.number(), self.number())
        }

        #[allow(dead_code)]
        fn vec2(&mut self) -> Vec2 {
            Vec2::new(self.number(), self.number())
        }
    }

    let mut cursor = Cursor {
        tokens,
        index: 0,
        error: String::new(),
    };

    let mut scene = Scene::default();
    let mut textures: HashMap<String, usize> = HashMap::new();
    let mut saw_camera = false;

    let material_index = |scene: &Scene, cursor: &mut Cursor, name: &str| -> usize {
        if let Some(&index) = scene.material_of.get(name) {
            index
        } else {
            cursor.error = format!("未知材质: {}", name);
            0
        }
    };

    while cursor.error.is_empty() && cursor.index < cursor.tokens.len() {
        let command = cursor.take().to_string();
        match command.as_str() {
            "camera" => {
                if !cursor.need(10) {
                    break;
                }
                scene.camera.eye = cursor.vec3();
                scene.camera.target = cursor.vec3();
                scene.camera.up = cursor.vec3();
                scene.camera.fov_y_degrees = cursor.number();
                saw_camera = true;
            }
            "ambient" => {
                scene.ambient = cursor.vec3();
            }
            "background" => {
                let color = cursor.vec3();
                scene.background = Color::new(color.x as u8, color.y as u8, color.z as u8);
            }
            "sky" => {
                scene.sky = cursor.number() != 0.0;
            }
            "point_light" => {
                let mut light = PointLight::default();
                light.position = cursor.vec3();
                light.color = cursor.vec3();
                light.intensity = cursor.number();
                if cursor.error.is_empty() {
                    scene.point_lights.push(light);
                }
            }
            "area_light" => {
                let mut light = AreaLight {
                    corner: Vec3::default(),
                    edge_u: Vec3::new(1.0, 0.0, 0.0),
                    edge_v: Vec3::new(0.0, 0.0, 1.0),
                    emission: Vec3::new(1.0, 1.0, 1.0),
                };
                light.corner = cursor.vec3();
                light.edge_u = cursor.vec3();
                light.edge_v = cursor.vec3();
                light.emission = cursor.vec3();
                if cursor.error.is_empty() {
                    scene.area_lights.push(light);
                }
            }
            "material" => {
                if !cursor.need(2) {
                    break;
                }
                let mut material = Material::default();
                material.name = cursor.take().to_string();
                let type_name = cursor.take().to_string();
                match type_name.as_str() {
                    "diffuse" => {
                        material.kind = MaterialType::Diffuse;
                        material.albedo = cursor.vec3();
                        if !cursor.need(1) {
                            break;
                        }
                        let texture = cursor.take().to_string();
                        if texture != "none" {
                            if let Some(&index) = textures.get(&texture) {
                                material.texture = index as i32;
                            } else {
                                match crate::image::Image::read_ppm(&resolve_asset(&texture)) {
                                    Ok(image) => {
                                        let mut stored = Texture::default();
                                        stored.image = image;
                                        material.texture = scene.textures.len() as i32;
                                        textures.insert(texture.clone(), material.texture as usize);
                                        scene.textures.push(stored);
                                    }
                                    Err(e) => {
                                        cursor.error = e;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    "metal" => {
                        material.kind = MaterialType::Metal;
                        material.albedo = cursor.vec3();
                        material.roughness = cursor.number();
                    }
                    "dielectric" => {
                        material.kind = MaterialType::Dielectric;
                        material.albedo = cursor.vec3();
                        material.ior = cursor.number();
                    }
                    "emissive" => {
                        material.kind = MaterialType::Emissive;
                        material.emission = cursor.vec3();
                        material.albedo = material.emission;
                    }
                    _ => {
                        cursor.error = format!("未知材质类型: {}", type_name);
                    }
                }
                if cursor.error.is_empty() {
                    if scene.material_of.contains_key(&material.name) {
                        cursor.error = format!("材质重复: {}", material.name);
                    } else {
                        scene.material_of.insert(material.name.clone(), scene.materials.len());
                        scene.materials.push(material);
                    }
                }
            }
            "mesh" => {
                if !cursor.need(9) {
                    break;
                }
                let material_name = cursor.take().to_string();
                let material_index = material_index(&scene, &mut cursor, &material_name);
                let mut instance = MeshInstance {
                    mesh: Mesh::default(),
                    transform: Transform::default(),
                    material: material_index,
                };
                instance.transform.translation = cursor.vec3();
                instance.transform.scale = cursor.vec3();
                instance.transform.rotate_y_degrees = cursor.number();
                if !cursor.need(1) {
                    break;
                }
                let file = resolve_asset(&cursor.take().to_string());
                match load_obj(&file) {
                    Ok(mesh) => {
                        for surface in &mesh.surfaces {
                            if !surface.material.is_empty() && !scene.material_of.contains_key(&surface.material) {
                                cursor.error = format!(
                                    "OBJ 引用了未定义材质 {} ({})",
                                    surface.material,
                                    file.display()
                                );
                            }
                        }
                        instance.mesh = mesh;
                    }
                    Err(e) => {
                        cursor.error = e;
                        break;
                    }
                }
                if cursor.error.is_empty() {
                    scene.meshes.push(instance);
                }
            }
            "sphere" => {
                if !cursor.need(5) {
                    break;
                }
                let material_name = cursor.take().to_string();
                let material_index = material_index(&scene, &mut cursor, &material_name);
                let mut sphere = SphereInstance {
                    center: Vec3::default(),
                    radius: 1.0,
                    material: material_index,
                };
                sphere.center = cursor.vec3();
                sphere.radius = cursor.number();
                if cursor.error.is_empty() {
                    scene.spheres.push(sphere);
                }
            }
            _ => {
                cursor.error = format!("未知场景命令: {}", command);
            }
        }
    }

    if cursor.error.is_empty() && !saw_camera {
        cursor.error = "场景缺少 camera".to_string();
    }
    if !cursor.error.is_empty() {
        return Err(cursor.error);
    }
    Ok(scene)
}
