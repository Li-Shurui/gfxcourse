use crate::vec::*;

#[derive(Clone, Copy, Debug)]
pub struct Mat3 {
    pub c: [Vec3; 3],
}

impl Default for Mat3 {
    fn default() -> Self {
        Self {
            c: [Vec3::new(1.0, 0.0, 0.0), Vec3::new(0.0, 1.0, 0.0), Vec3::new(0.0, 0.0, 1.0)],
        }
    }
}

impl Mat3 {
    pub fn identity() -> Self {
        Self::default()
    }

    pub fn translation(offset: Vec2) -> Self {
        let mut m = Self::identity();
        m.c[2] = Vec3::new(offset.x, offset.y, 1.0);
        m
    }

    pub fn scale(s: Vec2) -> Self {
        let mut m = Self::identity();
        m.c[0] = Vec3::new(s.x, 0.0, 0.0);
        m.c[1] = Vec3::new(0.0, s.y, 0.0);
        m
    }

    pub fn rotation(radians_value: f32) -> Self {
        let cs = radians_value.cos();
        let sn = radians_value.sin();
        let mut m = Self::identity();
        m.c[0] = Vec3::new(cs, sn, 0.0);
        m.c[1] = Vec3::new(-sn, cs, 0.0);
        m
    }

    pub fn mul(&self, v: Vec3) -> Vec3 {
        self.c[0] * v.x + self.c[1] * v.y + self.c[2] * v.z
    }

    pub fn transform_point(&self, p: Vec2) -> Vec2 {
        let r = self.mul(Vec3::new(p.x, p.y, 1.0));
        Vec2::new(r.x, r.y)
    }

    pub fn transform_vector(&self, v: Vec3) -> Vec3 {
        self.c[0] * v.x + self.c[1] * v.y + self.c[2] * v.z
    }
}

impl std::ops::Mul for Mat3 {
    type Output = Self;
    fn mul(self, b: Self) -> Self::Output {
        let mut out = Self::default();
        out.c[0] = self.transform_vector(b.c[0]);
        out.c[1] = self.transform_vector(b.c[1]);
        out.c[2] = self.transform_vector(b.c[2]);
        out
    }
}

pub fn transpose(m: Mat3) -> Mat3 {
    let mut out = Mat3::default();
    out.c[0] = Vec3::new(m.c[0].x, m.c[1].x, m.c[2].x);
    out.c[1] = Vec3::new(m.c[0].y, m.c[1].y, m.c[2].y);
    out.c[2] = Vec3::new(m.c[0].z, m.c[1].z, m.c[2].z);
    out
}

pub fn inverse3(m: Mat3) -> Option<Mat3> {
    let det = dot3(m.c[0], cross(m.c[1], m.c[2]));
    if det.abs() < 1e-8 {
        return None;
    }
    let inv_det = 1.0 / det;
    let r0 = cross(m.c[1], m.c[2]) * inv_det;
    let r1 = cross(m.c[2], m.c[0]) * inv_det;
    let r2 = cross(m.c[0], m.c[1]) * inv_det;
    let mut out = Mat3::default();
    out.c[0] = Vec3::new(r0.x, r1.x, r2.x);
    out.c[1] = Vec3::new(r0.y, r1.y, r2.y);
    out.c[2] = Vec3::new(r0.z, r1.z, r2.z);
    Some(out)
}

#[derive(Clone, Copy, Debug)]
pub struct Mat4 {
    pub m: [f32; 16],
}

impl Default for Mat4 {
    fn default() -> Self {
        Self {
            m: [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0],
        }
    }
}

impl Mat4 {
    pub fn identity() -> Self {
        Self::default()
    }

    pub fn zero() -> Self {
        Self { m: [0.0; 16] }
    }

    #[inline]
    pub fn get(&self, row: usize, col: usize) -> f32 {
        self.m[col * 4 + row]
    }

    #[inline]
    pub fn set(&mut self, row: usize, col: usize, value: f32) {
        self.m[col * 4 + row] = value;
    }

    pub fn translation(offset: Vec3) -> Self {
        let mut out = Self::identity();
        out.set(0, 3, offset.x);
        out.set(1, 3, offset.y);
        out.set(2, 3, offset.z);
        out
    }

    pub fn scale(s: Vec3) -> Self {
        let mut out = Self::identity();
        out.set(0, 0, s.x);
        out.set(1, 1, s.y);
        out.set(2, 2, s.z);
        out
    }

    pub fn rotation(axis: Vec3, radians_value: f32) -> Self {
        let u = normalize3(axis);
        let cs = radians_value.cos();
        let sn = radians_value.sin();
        let t = 1.0 - cs;
        let mut out = Self::zero();
        out.set(3, 3, 1.0);
        out.set(0, 0, t * u.x * u.x + cs);
        out.set(1, 0, t * u.x * u.y + sn * u.z);
        out.set(2, 0, t * u.x * u.z - sn * u.y);
        out.set(0, 1, t * u.x * u.y - sn * u.z);
        out.set(1, 1, t * u.y * u.y + cs);
        out.set(2, 1, t * u.y * u.z + sn * u.x);
        out.set(0, 2, t * u.x * u.z + sn * u.y);
        out.set(1, 2, t * u.y * u.z - sn * u.x);
        out.set(2, 2, t * u.z * u.z + cs);
        out
    }

    pub fn transform_point(&self, p: Vec3) -> Vec3 {
        (*self * vec4(p, 1.0)).xyz()
    }

    pub fn transform_vector(&self, v: Vec3) -> Vec3 {
        (*self * vec4(v, 0.0)).xyz()
    }

    pub fn upper3(&self) -> Mat3 {
        let mut out = Mat3::default();
        out.c[0] = Vec3::new(self.get(0, 0), self.get(1, 0), self.get(2, 0));
        out.c[1] = Vec3::new(self.get(0, 1), self.get(1, 1), self.get(2, 1));
        out.c[2] = Vec3::new(self.get(0, 2), self.get(1, 2), self.get(2, 2));
        out
    }
}

impl std::ops::Mul<Vec4> for Mat4 {
    type Output = Vec4;
    fn mul(self, v: Vec4) -> Self::Output {
        Vec4::new(
            self.get(0, 0) * v.x + self.get(0, 1) * v.y + self.get(0, 2) * v.z + self.get(0, 3) * v.w,
            self.get(1, 0) * v.x + self.get(1, 1) * v.y + self.get(1, 2) * v.z + self.get(1, 3) * v.w,
            self.get(2, 0) * v.x + self.get(2, 1) * v.y + self.get(2, 2) * v.z + self.get(2, 3) * v.w,
            self.get(3, 0) * v.x + self.get(3, 1) * v.y + self.get(3, 2) * v.z + self.get(3, 3) * v.w,
        )
    }
}

impl std::ops::Mul for Mat4 {
    type Output = Self;
    fn mul(self, b: Self) -> Self::Output {
        let mut out = Self::default();
        for col in 0..4 {
            for row in 0..4 {
                let mut sum = 0.0;
                for k in 0..4 {
                    sum += self.get(row, k) * b.get(k, col);
                }
                out.set(row, col, sum);
            }
        }
        out
    }
}

pub fn inverse4(in_m: Mat4) -> Option<Mat4> {
    let mut a = [[0.0f32; 8]; 4];
    for row in 0..4 {
        for col in 0..4 {
            a[row][col] = in_m.get(row, col);
        }
        a[row][row + 4] = 1.0;
    }
    for col in 0..4 {
        let mut pivot = col;
        for row in (col + 1)..4 {
            if a[row][col].abs() > a[pivot][col].abs() {
                pivot = row;
            }
        }
        if a[pivot][col].abs() < 1e-8 {
            return None;
        }
        if pivot != col {
            a.swap(col, pivot);
        }
        let div = a[col][col];
        for k in 0..8 {
            a[col][k] /= div;
        }
        for row in 0..4 {
            if row == col {
                continue;
            }
            let factor = a[row][col];
            for k in 0..8 {
                a[row][k] -= factor * a[col][k];
            }
        }
    }
    let mut out = Mat4::default();
    for row in 0..4 {
        for col in 0..4 {
            out.set(row, col, a[row][col + 4]);
        }
    }
    Some(out)
}

pub fn normal_matrix(model: Mat4) -> Mat3 {
    let linear = model.upper3();
    if let Some(inv) = inverse3(linear) {
        transpose(inv)
    } else {
        linear
    }
}
