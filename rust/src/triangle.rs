use crate::image::{Color, Image};
use crate::vec::{Vec2, Vec3};

#[inline]
pub fn edge(a: Vec2, b: Vec2, p: Vec2) -> f32 {
    (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)
}

#[inline]
pub fn is_top_left(start: Vec2, end: Vec2) -> bool {
    (start.y == end.y && start.x > end.x) || (end.y > start.y)
}

#[derive(Clone, Copy, Debug)]
pub struct Barycentric {
    pub w0: f32,
    pub w1: f32,
    pub w2: f32,
    pub inside: bool,
}

pub fn barycentric(p: Vec2, a: Vec2, b: Vec2, c: Vec2) -> Barycentric {
    let signed_area = edge(a, b, c);
    if signed_area == 0.0 {
        return Barycentric {
            w0: 0.0,
            w1: 0.0,
            w2: 0.0,
            inside: false,
        };
    }
    let inv_area = 1.0 / signed_area;
    let w0 = edge(b, c, p) * inv_area;
    let w1 = edge(c, a, p) * inv_area;
    let w2 = edge(a, b, p) * inv_area;

    let (s0, mut s1, mut s2) = (a, b, c);
    if signed_area < 0.0 {
        std::mem::swap(&mut s1, &mut s2);
    }
    let e0 = edge(s1, s2, p);
    let e1 = edge(s2, s0, p);
    let e2 = edge(s0, s1, p);
    let accept = |w: f32, top_left: bool| w > 0.0 || (w == 0.0 && top_left);
    let inside = accept(e0, is_top_left(s1, s2)) && accept(e1, is_top_left(s2, s0)) && accept(e2, is_top_left(s0, s1));
    Barycentric { w0, w1, w2, inside }
}

pub fn fill_triangle(image: &mut Image, a: Vec2, b: Vec2, c: Vec2, color: Color) {
    if !a.x.is_finite() || !b.x.is_finite() || !c.x.is_finite() {
        return;
    }
    let min_x = a.x.min(b.x).min(c.x);
    let max_x = a.x.max(b.x).max(c.x);
    let min_y = a.y.min(b.y).min(c.y);
    let max_y = a.y.max(b.y).max(c.y);
    let x0 = (min_x.floor() as i32).clamp(0, image.width() as i32 - 1) as usize;
    let x1 = (max_x.floor() as i32).clamp(0, image.width() as i32 - 1) as usize;
    let y0 = (min_y.floor() as i32).clamp(0, image.height() as i32 - 1) as usize;
    let y1 = (max_y.floor() as i32).clamp(0, image.height() as i32 - 1) as usize;

    for y in y0..=y1 {
        for x in x0..=x1 {
            let p = Vec2::new(x as f32 + 0.5, y as f32 + 0.5);
            if barycentric(p, a, b, c).inside {
                image.put(x, y, color);
            }
        }
    }
}

pub fn fill_triangle_colored(image: &mut Image, a: Vec2, b: Vec2, c: Vec2, ca: Vec3, cb: Vec3, cc: Vec3) {
    if !a.x.is_finite() || !b.x.is_finite() || !c.x.is_finite() {
        return;
    }
    let min_x = a.x.min(b.x).min(c.x);
    let max_x = a.x.max(b.x).max(c.x);
    let min_y = a.y.min(b.y).min(c.y);
    let max_y = a.y.max(b.y).max(c.y);
    let x0 = (min_x.floor() as i32).clamp(0, image.width() as i32 - 1) as usize;
    let x1 = (max_x.floor() as i32).clamp(0, image.width() as i32 - 1) as usize;
    let y0 = (min_y.floor() as i32).clamp(0, image.height() as i32 - 1) as usize;
    let y1 = (max_y.floor() as i32).clamp(0, image.height() as i32 - 1) as usize;

    for y in y0..=y1 {
        for x in x0..=x1 {
            let p = Vec2::new(x as f32 + 0.5, y as f32 + 0.5);
            let bc = barycentric(p, a, b, c);
            if bc.inside {
                let color = ca * bc.w0 + cb * bc.w1 + cc * bc.w2;
                crate::color::write_linear(image, x, y, color);
            }
        }
    }
}
