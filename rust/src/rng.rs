use crate::vec::{length_squared3, Vec3};

pub struct Rng {
    state: u64,
}

impl Rng {
    pub fn new(seed: u64) -> Self {
        Self {
            state: if seed == 0 { 0x9e3779b97f4a7c15 } else { seed },
        }
    }

    pub fn next_u32(&mut self) -> u32 {
        self.state = self.state.wrapping_add(0x9e3779b97f4a7c15);
        let mut z = self.state;
        z = (z ^ (z >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
        z = (z ^ (z >> 27)).wrapping_mul(0x94d049bb133111eb);
        (z ^ (z >> 31)) as u32
    }

    pub fn uniform(&mut self) -> f32 {
        (self.next_u32() >> 8) as f32 * (1.0 / 16777216.0)
    }

    pub fn in_unit_sphere(&mut self) -> Vec3 {
        loop {
            let point = Vec3::new(
                self.uniform() * 2.0 - 1.0,
                self.uniform() * 2.0 - 1.0,
                self.uniform() * 2.0 - 1.0,
            );
            if length_squared3(point) < 1.0 {
                return point;
            }
        }
    }
}

pub fn sample_cosine_hemisphere(normal: Vec3, rng: &mut Rng) -> Vec3 {
    let u1 = rng.uniform();
    let u2 = rng.uniform();
    let radius = u1.sqrt();
    let phi = 2.0 * crate::vec::K_PI * u2;
    let local = Vec3::new(
        radius * phi.cos(),
        (0.0f32.max(1.0 - u1)).sqrt(),
        radius * phi.sin(),
    );
    let helper = if normal.y.abs() < 0.999 {
        Vec3::new(0.0, 1.0, 0.0)
    } else {
        Vec3::new(1.0, 0.0, 0.0)
    };
    let tangent = crate::vec::normalize3(crate::vec::cross(helper, normal));
    let bitangent = crate::vec::cross(normal, tangent);
    tangent * local.x + normal * local.y + bitangent * local.z
}
