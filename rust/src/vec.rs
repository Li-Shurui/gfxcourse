use std::f32::consts::PI;
use std::ops::{Add, AddAssign, Div, Index, IndexMut, Mul, Neg, Sub};

pub const K_PI: f32 = PI;

#[inline]
pub const fn radians(degrees: f32) -> f32 {
    degrees * K_PI / 180.0
}

#[inline]
pub const fn degrees(radians_value: f32) -> f32 {
    radians_value * 180.0 / K_PI
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vec2 {
    pub x: f32,
    pub y: f32,
}

impl Vec2 {
    #[inline]
    pub const fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
}

impl Index<usize> for Vec2 {
    type Output = f32;
    #[inline]
    fn index(&self, axis: usize) -> &Self::Output {
        match axis {
            0 => &self.x,
            1 => &self.y,
            _ => panic!("Vec2 index out of range"),
        }
    }
}

impl IndexMut<usize> for Vec2 {
    #[inline]
    fn index_mut(&mut self, axis: usize) -> &mut Self::Output {
        match axis {
            0 => &mut self.x,
            1 => &mut self.y,
            _ => panic!("Vec2 index out of range"),
        }
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vec3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl Vec3 {
    #[inline]
    pub const fn new(x: f32, y: f32, z: f32) -> Self {
        Self { x, y, z }
    }

    #[inline]
    pub const fn xy(&self) -> Vec2 {
        Vec2::new(self.x, self.y)
    }
}

impl Index<usize> for Vec3 {
    type Output = f32;
    #[inline]
    fn index(&self, axis: usize) -> &Self::Output {
        match axis {
            0 => &self.x,
            1 => &self.y,
            2 => &self.z,
            _ => panic!("Vec3 index out of range"),
        }
    }
}

impl IndexMut<usize> for Vec3 {
    #[inline]
    fn index_mut(&mut self, axis: usize) -> &mut Self::Output {
        match axis {
            0 => &mut self.x,
            1 => &mut self.y,
            2 => &mut self.z,
            _ => panic!("Vec3 index out of range"),
        }
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vec4 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

impl Vec4 {
    #[inline]
    pub const fn new(x: f32, y: f32, z: f32, w: f32) -> Self {
        Self { x, y, z, w }
    }

    #[inline]
    pub const fn xyz(&self) -> Vec3 {
        Vec3::new(self.x, self.y, self.z)
    }
}

impl Index<usize> for Vec4 {
    type Output = f32;
    #[inline]
    fn index(&self, axis: usize) -> &Self::Output {
        match axis {
            0 => &self.x,
            1 => &self.y,
            2 => &self.z,
            3 => &self.w,
            _ => panic!("Vec4 index out of range"),
        }
    }
}

impl IndexMut<usize> for Vec4 {
    #[inline]
    fn index_mut(&mut self, axis: usize) -> &mut Self::Output {
        match axis {
            0 => &mut self.x,
            1 => &mut self.y,
            2 => &mut self.z,
            3 => &mut self.w,
            _ => panic!("Vec4 index out of range"),
        }
    }
}

impl Add for Vec2 {
    type Output = Self;
    #[inline]
    fn add(self, rhs: Self) -> Self::Output {
        Self::new(self.x + rhs.x, self.y + rhs.y)
    }
}

impl Sub for Vec2 {
    type Output = Self;
    #[inline]
    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(self.x - rhs.x, self.y - rhs.y)
    }
}

impl Neg for Vec2 {
    type Output = Self;
    #[inline]
    fn neg(self) -> Self::Output {
        Self::new(-self.x, -self.y)
    }
}

impl Mul<f32> for Vec2 {
    type Output = Self;
    #[inline]
    fn mul(self, s: f32) -> Self::Output {
        Self::new(self.x * s, self.y * s)
    }
}

impl Mul<Vec2> for f32 {
    type Output = Vec2;
    #[inline]
    fn mul(self, a: Vec2) -> Self::Output {
        a * self
    }
}

impl Div<f32> for Vec2 {
    type Output = Self;
    #[inline]
    fn div(self, s: f32) -> Self::Output {
        Self::new(self.x / s, self.y / s)
    }
}

impl Mul<Vec2> for Vec2 {
    type Output = Self;
    #[inline]
    fn mul(self, rhs: Vec2) -> Self::Output {
        Self::new(self.x * rhs.x, self.y * rhs.y)
    }
}

impl Add for Vec3 {
    type Output = Self;
    #[inline]
    fn add(self, rhs: Self) -> Self::Output {
        Self::new(self.x + rhs.x, self.y + rhs.y, self.z + rhs.z)
    }
}

impl Sub for Vec3 {
    type Output = Self;
    #[inline]
    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(self.x - rhs.x, self.y - rhs.y, self.z - rhs.z)
    }
}

impl Neg for Vec3 {
    type Output = Self;
    #[inline]
    fn neg(self) -> Self::Output {
        Self::new(-self.x, -self.y, -self.z)
    }
}

impl Mul<f32> for Vec3 {
    type Output = Self;
    #[inline]
    fn mul(self, s: f32) -> Self::Output {
        Self::new(self.x * s, self.y * s, self.z * s)
    }
}

impl Mul<Vec3> for f32 {
    type Output = Vec3;
    #[inline]
    fn mul(self, a: Vec3) -> Self::Output {
        a * self
    }
}

impl Div<f32> for Vec3 {
    type Output = Self;
    #[inline]
    fn div(self, s: f32) -> Self::Output {
        Self::new(self.x / s, self.y / s, self.z / s)
    }
}

impl Mul<Vec3> for Vec3 {
    type Output = Self;
    #[inline]
    fn mul(self, rhs: Vec3) -> Self::Output {
        Self::new(self.x * rhs.x, self.y * rhs.y, self.z * rhs.z)
    }
}

impl AddAssign for Vec3 {
    #[inline]
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl Add for Vec4 {
    type Output = Self;
    #[inline]
    fn add(self, rhs: Self) -> Self::Output {
        Self::new(self.x + rhs.x, self.y + rhs.y, self.z + rhs.z, self.w + rhs.w)
    }
}

impl Sub for Vec4 {
    type Output = Self;
    #[inline]
    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(self.x - rhs.x, self.y - rhs.y, self.z - rhs.z, self.w - rhs.w)
    }
}

impl Mul<f32> for Vec4 {
    type Output = Self;
    #[inline]
    fn mul(self, s: f32) -> Self::Output {
        Self::new(self.x * s, self.y * s, self.z * s, self.w * s)
    }
}

impl Mul<Vec4> for f32 {
    type Output = Vec4;
    #[inline]
    fn mul(self, a: Vec4) -> Self::Output {
        a * self
    }
}

#[inline]
pub fn dot(a: Vec2, b: Vec2) -> f32 {
    a.x * b.x + a.y * b.y
}

#[inline]
pub fn dot3(a: Vec3, b: Vec3) -> f32 {
    a.x * b.x + a.y * b.y + a.z * b.z
}

#[inline]
pub fn dot4(a: Vec4, b: Vec4) -> f32 {
    a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w
}

#[inline]
pub fn cross(a: Vec3, b: Vec3) -> Vec3 {
    Vec3::new(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    )
}

#[inline]
pub fn length_squared(a: Vec2) -> f32 {
    dot(a, a)
}

#[inline]
pub fn length_squared3(a: Vec3) -> f32 {
    dot3(a, a)
}

#[inline]
pub fn length(a: Vec2) -> f32 {
    length_squared(a).sqrt()
}

#[inline]
pub fn length3(a: Vec3) -> f32 {
    length_squared3(a).sqrt()
}

#[inline]
pub fn normalize(a: Vec2) -> Vec2 {
    let len = length(a);
    if len <= 1e-20 {
        Vec2::default()
    } else {
        a / len
    }
}

#[inline]
pub fn normalize3(a: Vec3) -> Vec3 {
    let len = length3(a);
    if len <= 1e-20 {
        Vec3::default()
    } else {
        a / len
    }
}

#[inline]
pub fn min(a: Vec3, b: Vec3) -> Vec3 {
    Vec3::new(a.x.min(b.x), a.y.min(b.y), a.z.min(b.z))
}

#[inline]
pub fn max(a: Vec3, b: Vec3) -> Vec3 {
    Vec3::new(a.x.max(b.x), a.y.max(b.y), a.z.max(b.z))
}

#[inline]
pub fn clamp_vec(v: Vec3, lo: f32, hi: f32) -> Vec3 {
    Vec3::new(v.x.clamp(lo, hi), v.y.clamp(lo, hi), v.z.clamp(lo, hi))
}

#[inline]
pub const fn vec4(v: Vec3, w: f32) -> Vec4 {
    Vec4::new(v.x, v.y, v.z, w)
}

#[inline]
pub const fn gray(value: f32) -> Vec3 {
    Vec3::new(value, value, value)
}

#[inline]
pub fn reflect(v: Vec3, n: Vec3) -> Vec3 {
    v - 2.0 * dot3(v, n) * n
}

#[inline]
pub fn refract(incident: Vec3, n: Vec3, eta: f32, transmitted: &mut Vec3) -> bool {
    let cos_i = (-dot3(incident, n)).min(1.0);
    let sin2_t = eta * eta * (1.0 - cos_i * cos_i);
    if sin2_t > 1.0 {
        return false;
    }
    let cos_t = (1.0 - sin2_t).sqrt();
    *transmitted = eta * incident + (eta * cos_i - cos_t) * n;
    true
}

#[inline]
pub fn fresnel_dielectric(cos_i: f32, eta_i: f32, eta_t: f32) -> f32 {
    let cos_i = cos_i.clamp(0.0, 1.0);
    let sin2_t = (eta_i / eta_t) * (eta_i / eta_t) * (1.0 - cos_i * cos_i);
    if sin2_t >= 1.0 {
        return 1.0;
    }
    let cos_t = (1.0 - sin2_t).sqrt();
    let rs = (eta_i * cos_i - eta_t * cos_t) / (eta_i * cos_i + eta_t * cos_t);
    let rp = (eta_i * cos_t - eta_t * cos_i) / (eta_i * cos_t + eta_t * cos_i);
    0.5 * (rs * rs + rp * rp)
}

#[inline]
pub fn lerp(a: Vec2, b: Vec2, t: f32) -> Vec2 {
    a * (1.0 - t) + b * t
}

#[inline]
pub fn lerp3(a: Vec3, b: Vec3, t: f32) -> Vec3 {
    a * (1.0 - t) + b * t
}
