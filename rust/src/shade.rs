use crate::camera::{view_projection, Camera};
use crate::image::Image;
use crate::mat::{normal_matrix, Mat4};
use crate::raster::{draw_clipped_triangle, ClipVert, DepthBuffer};
use crate::shapes::Geometry;
use crate::vec::{vec4, Vec3};

pub fn draw_geometry<S: FnMut(&crate::raster::Varying) -> Vec3>(
    image: &mut Image,
    depth: &mut DepthBuffer,
    camera: &Camera,
    model: &Mat4,
    geometry: &Geometry,
    mut shader: S,
) {
    let aspect = image.width() as f32 / image.height() as f32;
    let view_proj = view_projection(camera, aspect);
    let normals = normal_matrix(*model);
    let mut i = 0;
    while i + 2 < geometry.indices.len() {
        let mut verts: [ClipVert; 3] = [ClipVert::default(), ClipVert::default(), ClipVert::default()];
        let mut ok = true;
        for k in 0..3 {
            let index = geometry.indices[i + k] as usize;
            if index >= geometry.positions.len() {
                ok = false;
                break;
            }
            verts[k].world = model.transform_point(geometry.positions[index]);
            verts[k].normal = normals.transform_vector(geometry.normals[index]);
            verts[k].uv = geometry.uvs[index];
            verts[k].color = Vec3::new(1.0, 1.0, 1.0);
            verts[k].clip = view_proj * vec4(verts[k].world, 1.0);
        }
        if ok {
            draw_clipped_triangle(image, depth, verts[0], verts[1], verts[2], &mut shader);
        }
        i += 3;
    }
}
