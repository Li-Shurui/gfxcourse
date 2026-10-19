use crate::camera::view_projection;
use crate::image::Image;
use crate::light::blinn_phong;
use crate::mat::normal_matrix;
use crate::material::{Material, MaterialType};
use crate::mesh::{make_uv_sphere, Mesh};
use crate::raster::{draw_clipped_triangle, ClipVert, DepthBuffer, Varying};
use crate::scene::{MeshInstance, Scene, SphereInstance, Transform};
use crate::vec::{gray, vec4, Vec3};

pub fn shade_fragment(frag: &Varying, material: &Material, scene: &Scene) -> Vec3 {
    let mut albedo = material.albedo;
    if material.texture >= 0 {
        albedo = albedo * scene.textures[material.texture as usize].sample(frag.uv);
    }
    if material.kind == MaterialType::Emissive {
        return material.emission;
    }

    let mut diffuse = albedo;
    let mut specular = gray(0.04);
    let mut shininess = 32.0f32;
    // 光栅器没有真正的反射/折射，金属和玻璃仍保留一部分漫反射，形状才看得清。
    if material.kind == MaterialType::Metal {
        diffuse = albedo * 0.45;
        specular = albedo;
        shininess = 24.0 + (192.0 - 24.0) * (1.0 - material.roughness.clamp(0.0, 1.0));
    } else if material.kind == MaterialType::Dielectric {
        diffuse = albedo * 0.55;
        specular = gray(0.85);
        shininess = 160.0;
    }
    blinn_phong(
        frag.world,
        frag.normal,
        scene.camera.eye,
        diffuse,
        specular,
        shininess,
        scene.ambient,
        &scene.point_lights,
    ) + material.emission
}

pub fn draw_mesh_instance(
    scene: &Scene,
    image: &mut Image,
    depth: &mut DepthBuffer,
    mesh: &Mesh,
    transform: &Transform,
    fallback_material: usize,
) {
    let aspect = image.width() as f32 / image.height().max(1) as f32;
    let view_proj = view_projection(&scene.camera, aspect);
    let model = transform.matrix();
    let normals = normal_matrix(model);

    for surface in &mesh.surfaces {
        let mut material_index = fallback_material;
        if !surface.material.is_empty() {
            if let Some(&found) = scene.material_of.get(&surface.material) {
                material_index = found;
            }
        }
        let material = scene.material(material_index);
        let mut i = 0;
        while i + 2 < surface.indices.len() {
            let mut verts: [ClipVert; 3] = [ClipVert::default(), ClipVert::default(), ClipVert::default()];
            let mut ok = true;
            for k in 0..3 {
                let index = surface.indices[i + k] as usize;
                if index >= mesh.vertices.len() {
                    ok = false;
                    break;
                }
                let vertex = &mesh.vertices[index];
                verts[k].world = model.transform_point(vertex.position);
                verts[k].normal = normals.transform_vector(vertex.normal);
                verts[k].uv = vertex.uv;
                verts[k].color = material.albedo;
                verts[k].clip = view_proj * vec4(verts[k].world, 1.0);
            }
            if ok {
                draw_clipped_triangle(image, depth, verts[0], verts[1], verts[2], |frag: &Varying| {
                    shade_fragment(frag, material, scene)
                });
            }
            i += 3;
        }
    }
}

pub fn render_raster(scene: &Scene, image: &mut Image) {
    image.clear(scene.background);
    let mut depth = DepthBuffer::new(image.width(), image.height());
    for instance in &scene.meshes {
        let MeshInstance {
            mesh,
            transform,
            material,
        } = instance;
        draw_mesh_instance(scene, image, &mut depth, mesh, transform, *material);
    }
    // 球体网格只生成一次；C++ 原版在每个球体实例的循环里重建，纯属浪费。
    let sphere_mesh = make_uv_sphere(48, 28, 1.0);
    for sphere in &scene.spheres {
        let SphereInstance {
            center,
            radius,
            material,
        } = sphere;
        let mut transform = Transform::default();
        transform.translation = *center;
        transform.scale = gray(*radius);
        draw_mesh_instance(scene, image, &mut depth, &sphere_mesh, &transform, *material);
    }
}
