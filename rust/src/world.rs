use crate::color::srgb_to_linear3;
use crate::material::Material;
use crate::mat::normal_matrix;
use crate::mesh::Vertex;
use crate::ray::{Hit, Ray};
use crate::scene::Scene;
use crate::sphere::Sphere;
use crate::trimesh::{intersect_triangle, Tri};
use crate::vec::{normalize3, Vec2, Vec3};

#[derive(Clone, Debug)]
pub struct RWorld<'a> {
    pub scene: &'a Scene,
    pub spheres: Vec<Sphere>,
    pub triangles: Vec<Tri>,
}

impl<'a> RWorld<'a> {
    pub fn from_scene(source: &'a Scene) -> Self {
        let mut world = Self {
            scene: source,
            spheres: Vec::new(),
            triangles: Vec::new(),
        };
        for sphere in &source.spheres {
            world.spheres.push(Sphere {
                center: sphere.center,
                radius: sphere.radius,
                material: sphere.material,
            });
        }
        for instance in &source.meshes {
            let model = instance.transform.matrix();
            let normals = normal_matrix(model);
            for surface in &instance.mesh.surfaces {
                let mut material = instance.material;
                if !surface.material.is_empty() {
                    if let Some(&found) = source.material_of.get(&surface.material) {
                        material = found;
                    }
                }
                for i in (0..surface.indices.len()).step_by(3) {
                    if i + 2 >= surface.indices.len() {
                        break;
                    }
                    let mut tri = Tri {
                        p: [Vec3::default(); 3],
                        n: [Vec3::default(); 3],
                        uv: [Vec2::default(); 3],
                        material,
                    };
                    for k in 0..3 {
                        let vertex: &Vertex = &instance.mesh.vertices[surface.indices[i + k] as usize];
                        tri.p[k] = model.transform_point(vertex.position);
                        tri.n[k] = normalize3(normals.transform_vector(vertex.normal));
                        tri.uv[k] = vertex.uv;
                    }
                    world.triangles.push(tri);
                }
            }
        }
        world
    }

    pub fn material(&self, index: usize) -> &Material {
        self.scene.material(index)
    }

    pub fn shade_albedo(&self, mat: &Material, uv: Vec2) -> Vec3 {
        let mut albedo = mat.albedo;
        if mat.texture >= 0 {
            albedo = albedo * self.scene.textures[mat.texture as usize].sample(uv);
        }
        albedo
    }

    pub fn miss(&self, ray: &Ray) -> Vec3 {
        if self.scene.sky {
            crate::ray::sky_color(ray)
        } else {
            srgb_to_linear3(self.scene.background)
        }
    }

    pub fn closest_hit(&self, ray: &Ray, tmin: f32, tmax: f32, hit: &mut Hit) -> bool {
        let mut found = false;
        let mut tmax = tmax;
        for sphere in &self.spheres {
            if sphere.intersect(ray, tmin, tmax, hit) {
                found = true;
                tmax = hit.t;
            }
        }
        for tri in &self.triangles {
            if intersect_triangle(ray, tri, tmin, tmax, hit) {
                found = true;
                tmax = hit.t;
            }
        }
        found
    }

}
