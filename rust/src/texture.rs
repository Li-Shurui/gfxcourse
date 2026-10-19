use crate::color::srgb_to_linear3;
use crate::image::{Color, Image};
use crate::vec::{gray, lerp3, Vec2, Vec3};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Filter {
    Nearest,
    Bilinear,
}

#[derive(Clone, Debug)]
pub struct Texture {
    pub image: Image,
    pub filter: Filter,
}

impl Default for Texture {
    fn default() -> Self {
        Self {
            image: Image::new(0, 0, Color::default()),
            filter: Filter::Bilinear,
        }
    }
}

impl Texture {
    fn fetch(&self, x: i32, y: i32) -> Vec3 {
        let w = self.image.width() as i32;
        let h = self.image.height() as i32;
        if w <= 0 || h <= 0 {
            return Vec3::default();
        }
        let wrap = |i: i32, n: i32| {
            let m = i % n;
            if m < 0 {
                m + n
            } else {
                m
            }
        };
        srgb_to_linear3(self.image.get(wrap(x, w) as usize, wrap(y, h) as usize))
    }

    pub fn sample(&self, uv: Vec2) -> Vec3 {
        if self.image.is_empty() {
            return gray(1.0);
        }
        let u = uv.x - uv.x.floor();
        let v = 1.0 - (uv.y - uv.y.floor());
        let width = self.image.width() as f32;
        let height = self.image.height() as f32;
        if self.filter == Filter::Nearest {
            let x = (u * width) as i32;
            let y = (v * height) as i32;
            let x = x.clamp(0, self.image.width() as i32 - 1) as usize;
            let y = y.clamp(0, self.image.height() as i32 - 1) as usize;
            return srgb_to_linear3(self.image.get(x, y));
        }

        let sx = u * width - 0.5;
        let sy = v * height - 0.5;
        let x0 = sx.floor() as i32;
        let y0 = sy.floor() as i32;
        let tx = sx - x0 as f32;
        let ty = sy - y0 as f32;
        let c00 = self.fetch(x0, y0);
        let c10 = self.fetch(x0 + 1, y0);
        let c01 = self.fetch(x0, y0 + 1);
        let c11 = self.fetch(x0 + 1, y0 + 1);
        let top = lerp3(c00, c10, tx);
        let bottom = lerp3(c01, c11, tx);
        lerp3(top, bottom, ty)
    }
}

pub fn make_checker(cells: usize, cell_size: usize, a: Color, b: Color) -> Texture {
    let n = cells * cell_size;
    let mut texture = Texture::default();
    texture.image = Image::new(n, n, Color::default());
    for y in 0..n {
        for x in 0..n {
            let on = ((x / cell_size) + (y / cell_size)) % 2 == 0;
            texture.image.put(x, y, if on { a } else { b });
        }
    }
    texture
}
