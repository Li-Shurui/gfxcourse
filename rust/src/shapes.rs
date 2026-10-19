use crate::vec::*;

#[derive(Clone, Debug, Default)]
pub struct Geometry {
    pub positions: Vec<Vec3>,
    pub normals: Vec<Vec3>,
    pub uvs: Vec<Vec2>,
    pub indices: Vec<u32>,
}

pub fn add_quad(geometry: &mut Geometry, a: Vec3, b: Vec3, c: Vec3, d: Vec3, normal: Vec3) {
    let base = geometry.positions.len() as u32;
    let corners = [a, b, c, d];
    let uvs = [Vec2::new(0.0, 0.0), Vec2::new(1.0, 0.0), Vec2::new(1.0, 1.0), Vec2::new(0.0, 1.0)];
    for i in 0..4 {
        geometry.positions.push(corners[i]);
        geometry.normals.push(normal);
        geometry.uvs.push(uvs[i]);
    }
    let face = [0u32, 1, 2, 0, 2, 3];
    for index in face {
        geometry.indices.push(base + index);
    }
}

pub fn make_cube() -> Geometry {
    let mut geometry = Geometry::default();
    add_quad(&mut geometry, Vec3::new(-1.0, -1.0, 1.0), Vec3::new(1.0, -1.0, 1.0), Vec3::new(1.0, 1.0, 1.0), Vec3::new(-1.0, 1.0, 1.0), Vec3::new(0.0, 0.0, 1.0));
    add_quad(&mut geometry, Vec3::new(1.0, -1.0, -1.0), Vec3::new(-1.0, -1.0, -1.0), Vec3::new(-1.0, 1.0, -1.0), Vec3::new(1.0, 1.0, -1.0), Vec3::new(0.0, 0.0, -1.0));
    add_quad(&mut geometry, Vec3::new(-1.0, -1.0, -1.0), Vec3::new(-1.0, -1.0, 1.0), Vec3::new(-1.0, 1.0, 1.0), Vec3::new(-1.0, 1.0, -1.0), Vec3::new(-1.0, 0.0, 0.0));
    add_quad(&mut geometry, Vec3::new(1.0, -1.0, 1.0), Vec3::new(1.0, -1.0, -1.0), Vec3::new(1.0, 1.0, -1.0), Vec3::new(1.0, 1.0, 1.0), Vec3::new(1.0, 0.0, 0.0));
    add_quad(&mut geometry, Vec3::new(-1.0, 1.0, 1.0), Vec3::new(1.0, 1.0, 1.0), Vec3::new(1.0, 1.0, -1.0), Vec3::new(-1.0, 1.0, -1.0), Vec3::new(0.0, 1.0, 0.0));
    add_quad(&mut geometry, Vec3::new(-1.0, -1.0, -1.0), Vec3::new(1.0, -1.0, -1.0), Vec3::new(1.0, -1.0, 1.0), Vec3::new(-1.0, -1.0, 1.0), Vec3::new(0.0, -1.0, 0.0));
    geometry
}

pub fn make_plane(half_extent: f32) -> Geometry {
    let mut geometry = Geometry::default();
    let h = half_extent;
    add_quad(&mut geometry, Vec3::new(-h, 0.0, -h), Vec3::new(h, 0.0, -h), Vec3::new(h, 0.0, h), Vec3::new(-h, 0.0, h), Vec3::new(0.0, 1.0, 0.0));
    geometry
}

pub fn make_sphere(slices: usize, stacks: usize) -> Geometry {
    let mut geometry = Geometry::default();
    for y in 0..=stacks {
        let v = y as f32 / stacks as f32;
        let phi = v * K_PI;
        for x in 0..=slices {
            let u = x as f32 / slices as f32;
            let theta = u * K_PI * 2.0;
            let normal = Vec3::new(phi.sin() * theta.cos(), phi.cos(), phi.sin() * theta.sin());
            geometry.positions.push(normal);
            geometry.normals.push(normal);
            geometry.uvs.push(Vec2::new(u, 1.0 - v));
        }
    }
    let index_at = |x: usize, y: usize| (y * (slices + 1) + x) as u32;
    for y in 0..stacks {
        for x in 0..slices {
            let i00 = index_at(x, y);
            let i10 = index_at(x + 1, y);
            let i01 = index_at(x, y + 1);
            let i11 = index_at(x + 1, y + 1);
            geometry.indices.extend_from_slice(&[i00, i11, i10, i00, i01, i11]);
        }
    }
    geometry
}
