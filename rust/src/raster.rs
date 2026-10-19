use crate::color::*;
use crate::image::Image;
use crate::triangle::*;
use crate::vec::*;

#[derive(Clone, Copy, Debug)]
pub struct Varying {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub inv_w: f32,
    pub world: Vec3,
    pub normal: Vec3,
    pub uv: Vec2,
    pub color: Vec3,
}

impl Default for Varying {
    fn default() -> Self {
        Self {
            x: 0.0,
            y: 0.0,
            z: 0.0,
            inv_w: 0.0,
            world: Vec3::default(),
            normal: Vec3::default(),
            uv: Vec2::default(),
            color: Vec3::new(1.0, 1.0, 1.0),
        }
    }
}

#[derive(Clone, Copy, Debug)]
pub struct ClipVert {
    pub clip: Vec4,
    pub world: Vec3,
    pub normal: Vec3,
    pub uv: Vec2,
    pub color: Vec3,
}

impl Default for ClipVert {
    fn default() -> Self {
        Self {
            clip: Vec4::default(),
            world: Vec3::default(),
            normal: Vec3::default(),
            uv: Vec2::default(),
            color: Vec3::new(1.0, 1.0, 1.0),
        }
    }
}

#[derive(Clone, Debug)]
pub struct DepthBuffer {
    pub width: usize,
    pub height: usize,
    pub z: Vec<f32>,
}

impl DepthBuffer {
    pub fn new(width: usize, height: usize) -> Self {
        Self {
            width,
            height,
            z: vec![f32::INFINITY; width * height],
        }
    }

    pub fn clear(&mut self) {
        self.z.fill(f32::INFINITY);
    }

    pub fn pass(&mut self, x: usize, y: usize, depth: f32) -> bool {
        if !depth.is_finite() {
            return false;
        }
        let slot = &mut self.z[y * self.width + x];
        if depth < *slot {
            *slot = depth;
            true
        } else {
            false
        }
    }
}

pub fn lerp_clip(a: &ClipVert, b: &ClipVert, t: f32) -> ClipVert {
    ClipVert {
        clip: a.clip + (b.clip - a.clip) * t,
        world: lerp3(a.world, b.world, t),
        normal: lerp3(a.normal, b.normal, t),
        uv: lerp(a.uv, b.uv, t),
        color: lerp3(a.color, b.color, t),
    }
}

fn clip_against<F: Fn(&ClipVert) -> f32>(polygon: &mut Vec<ClipVert>, dist: F) {
    if polygon.is_empty() {
        return;
    }
    let mut out = Vec::with_capacity(polygon.len() + 1);
    for i in 0..polygon.len() {
        let current = polygon[i];
        let previous = polygon[(i + polygon.len() - 1) % polygon.len()];
        let current_d = dist(&current);
        let previous_d = dist(&previous);
        let current_in = current_d >= 0.0;
        let previous_in = previous_d >= 0.0;
        if current_in != previous_in {
            let denom = previous_d - current_d;
            if denom.abs() > 1e-8 {
                out.push(lerp_clip(&previous, &current, previous_d / denom));
            }
        }
        if current_in {
            out.push(current);
        }
    }
    std::mem::swap(polygon, &mut out);
}

pub fn clip_frustum(polygon: &mut Vec<ClipVert>) {
    clip_against(polygon, |v| v.clip.x + v.clip.w);
    clip_against(polygon, |v| v.clip.w - v.clip.x);
    clip_against(polygon, |v| v.clip.y + v.clip.w);
    clip_against(polygon, |v| v.clip.w - v.clip.y);
    clip_against(polygon, |v| v.clip.z + v.clip.w);
    clip_against(polygon, |v| v.clip.w - v.clip.z);
}

pub fn to_varying(vertex: &ClipVert, width: usize, height: usize) -> Option<Varying> {
    if !(vertex.clip.w > 1e-6) || !vertex.clip.w.is_finite() {
        return None;
    }
    let inv_w = 1.0 / vertex.clip.w;
    let ndc_x = vertex.clip.x * inv_w;
    let ndc_y = vertex.clip.y * inv_w;
    let ndc_z = vertex.clip.z * inv_w;
    let x = (ndc_x + 1.0) * 0.5 * width as f32;
    let y = (1.0 - (ndc_y + 1.0) * 0.5) * height as f32;
    let z = ndc_z * 0.5 + 0.5;
    if !x.is_finite() || !y.is_finite() || !z.is_finite() {
        return None;
    }
    Some(Varying {
        x,
        y,
        z,
        inv_w,
        world: vertex.world,
        normal: vertex.normal,
        uv: vertex.uv,
        color: vertex.color,
    })
}

pub fn perspective_interpolate<T>(a: T, b: T, c: T, w0: f32, w1: f32, w2: f32, iw0: f32, iw1: f32, iw2: f32) -> T
where
    T: Copy + std::ops::Mul<f32, Output = T> + std::ops::Add<Output = T>,
{
    let inv_w = w0 * iw0 + w1 * iw1 + w2 * iw2;
    (a * (w0 * iw0) + b * (w1 * iw1) + c * (w2 * iw2)) * (1.0 / inv_w)
}

pub fn rasterize_triangle<S: FnMut(&Varying) -> Vec3>(
    image: &mut Image,
    depth: &mut DepthBuffer,
    v0: Varying,
    mut v1: Varying,
    mut v2: Varying,
    mut shader: S,
) {
    if edge(Vec2::new(v0.x, v0.y), Vec2::new(v1.x, v1.y), Vec2::new(v2.x, v2.y)) < 0.0 {
        std::mem::swap(&mut v1, &mut v2);
    }
    let a = Vec2::new(v0.x, v0.y);
    let b = Vec2::new(v1.x, v1.y);
    let c = Vec2::new(v2.x, v2.y);
    let area = edge(a, b, c);
    if !(area > 1e-6) {
        return;
    }

    let x0 = (a.x.min(b.x).min(c.x).floor() as i32).clamp(0, image.width() as i32 - 1) as usize;
    let x1 = (a.x.max(b.x).max(c.x).floor() as i32).clamp(0, image.width() as i32 - 1) as usize;
    let y0 = (a.y.min(b.y).min(c.y).floor() as i32).clamp(0, image.height() as i32 - 1) as usize;
    let y1 = (a.y.max(b.y).max(c.y).floor() as i32).clamp(0, image.height() as i32 - 1) as usize;
    let top0 = is_top_left(b, c);
    let top1 = is_top_left(c, a);
    let top2 = is_top_left(a, b);
    let inside = |w: f32, top_left: bool| w > 0.0 || (w == 0.0 && top_left);

    for y in y0..=y1 {
        for x in x0..=x1 {
            let p = Vec2::new(x as f32 + 0.5, y as f32 + 0.5);
            let e0 = edge(b, c, p);
            let e1 = edge(c, a, p);
            let e2 = edge(a, b, p);
            if !inside(e0, top0) || !inside(e1, top1) || !inside(e2, top2) {
                continue;
            }
            let w0 = e0 / area;
            let w1 = e1 / area;
            let w2 = e2 / area;
            let z = w0 * v0.z + w1 * v1.z + w2 * v2.z;
            if !depth.pass(x, y, z) {
                continue;
            }
            let denom = w0 * v0.inv_w + w1 * v1.inv_w + w2 * v2.inv_w;
            if !(denom.abs() > 1e-12) {
                continue;
            }
            let mut frag = Varying {
                x: p.x,
                y: p.y,
                z,
                inv_w: denom,
                world: perspective_interpolate(v0.world, v1.world, v2.world, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w),
                normal: perspective_interpolate(v0.normal, v1.normal, v2.normal, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w),
                uv: perspective_interpolate(v0.uv, v1.uv, v2.uv, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w),
                color: perspective_interpolate(v0.color, v1.color, v2.color, w0, w1, w2, v0.inv_w, v1.inv_w, v2.inv_w),
            };
            let normal_len = length3(frag.normal);
            if normal_len > 1e-8 {
                frag.normal = frag.normal / normal_len;
            }
            write_linear(image, x, y, shader(&frag));
        }
    }
}

pub fn draw_clipped_triangle<S: FnMut(&Varying) -> Vec3>(
    image: &mut Image,
    depth: &mut DepthBuffer,
    a: ClipVert,
    b: ClipVert,
    c: ClipVert,
    mut shader: S,
) {
    let mut polygon = vec![a, b, c];
    clip_frustum(&mut polygon);
    if polygon.len() < 3 {
        return;
    }

    let mut projected = Vec::with_capacity(polygon.len());
    for v in &polygon {
        match to_varying(v, image.width(), image.height()) {
            Some(p) => projected.push(p),
            None => return,
        }
    }
    for i in 1..projected.len() - 1 {
        rasterize_triangle(image, depth, projected[0], projected[i], projected[i + 1], &mut shader);
    }
}
