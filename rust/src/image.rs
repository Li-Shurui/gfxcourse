use crate::paths::output_file;
use std::fs::File;
use std::io::{BufReader, BufWriter, Read, Write};

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl Color {
    pub const fn new(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b }
    }
}

#[derive(Clone, Debug)]
pub struct Image {
    width: usize,
    height: usize,
    pixels: Vec<u8>,
}

impl Image {
    pub fn new(width: usize, height: usize, fill: Color) -> Self {
        let mut img = Self {
            width,
            height,
            pixels: vec![0; width * height * 3],
        };
        img.clear(fill);
        img
    }

    pub fn width(&self) -> usize {
        self.width
    }

    pub fn height(&self) -> usize {
        self.height
    }

    pub fn is_empty(&self) -> bool {
        self.width == 0 || self.height == 0
    }

    pub fn clear(&mut self, fill: Color) {
        for y in 0..self.height {
            for x in 0..self.width {
                self.put(x, y, fill);
            }
        }
    }

    #[inline]
    fn index(&self, x: usize, y: usize) -> usize {
        (y * self.width + x) * 3
    }

    pub fn put(&mut self, x: usize, y: usize, color: Color) {
        if x >= self.width || y >= self.height {
            return;
        }
        let i = self.index(x, y);
        self.pixels[i] = color.r;
        self.pixels[i + 1] = color.g;
        self.pixels[i + 2] = color.b;
    }

    pub fn get(&self, x: usize, y: usize) -> Color {
        if x >= self.width || y >= self.height {
            return Color::default();
        }
        let i = self.index(x, y);
        Color::new(self.pixels[i], self.pixels[i + 1], self.pixels[i + 2])
    }

    pub fn fill_rect(&mut self, x: usize, y: usize, w: usize, h: usize, color: Color) {
        for yy in y..y + h {
            for xx in x..x + w {
                self.put(xx, yy, color);
            }
        }
    }

    pub fn blit(&mut self, src: &Image, dst_x: usize, dst_y: usize) {
        for y in 0..src.height {
            for x in 0..src.width {
                self.put(dst_x + x, dst_y + y, src.get(x, y));
            }
        }
    }

    pub fn write_ppm(&self, path: &std::path::Path) -> std::io::Result<()> {
        let file = File::create(path)?;
        let mut out = BufWriter::new(file);
        writeln!(out, "P6")?;
        writeln!(out, "{} {}", self.width, self.height)?;
        writeln!(out, "255")?;
        out.write_all(&self.pixels)?;
        Ok(())
    }

    pub fn write_bmp(&self, path: &std::path::Path) -> std::io::Result<()> {
        let file = File::create(path)?;
        let mut out = BufWriter::new(file);

        let row_stride = (self.width * 3 + 3) & !3;
        let pixel_bytes = (row_stride * self.height) as u32;
        let file_size = 54u32 + pixel_bytes;

        write_u16(&mut out, 0x4D42)?;
        write_u32(&mut out, file_size)?;
        write_u16(&mut out, 0)?;
        write_u16(&mut out, 0)?;
        write_u32(&mut out, 54)?;
        write_u32(&mut out, 40)?;
        write_u32(&mut out, self.width as u32)?;
        write_u32(&mut out, self.height as u32)?;
        write_u16(&mut out, 1)?;
        write_u16(&mut out, 24)?;
        write_u32(&mut out, 0)?;
        write_u32(&mut out, pixel_bytes)?;
        write_u32(&mut out, 2835)?;
        write_u32(&mut out, 2835)?;
        write_u32(&mut out, 0)?;
        write_u32(&mut out, 0)?;

        let pad = vec![0u8; row_stride - self.width * 3];
        for y in (0..self.height).rev() {
            for x in 0..self.width {
                let c = self.get(x, y);
                out.write_all(&[c.b, c.g, c.r])?;
            }
            if !pad.is_empty() {
                out.write_all(&pad)?;
            }
        }
        Ok(())
    }

    pub fn read_ppm(path: &std::path::Path) -> Result<Image, String> {
        let file = File::open(path).map_err(|e| format!("无法打开 PPM: {}: {}", path.display(), e))?;
        let mut reader = BufReader::new(file);

        let magic = read_ppm_token(&mut reader).ok_or_else(|| format!("PPM 头无效: {}", path.display()))?;
        if magic != "P3" && magic != "P6" {
            return Err(format!("只支持 P3/P6 PPM: {}", path.display()));
        }
        let width = read_ppm_int(&mut reader).ok_or_else(|| format!("PPM 头无效: {}", path.display()))?;
        let height = read_ppm_int(&mut reader).ok_or_else(|| format!("PPM 头无效: {}", path.display()))?;
        let maxval = read_ppm_int(&mut reader).ok_or_else(|| format!("PPM 头无效: {}", path.display()))?;
        if width <= 0 || height <= 0 || maxval <= 0 || maxval > 255 {
            return Err(format!("PPM 头无效: {}", path.display()));
        }

        let mut image = Image::new(width as usize, height as usize, Color::default());
        if magic == "P6" {
            let mut raw = vec![0u8; width as usize * height as usize * 3];
            reader.read_exact(&mut raw).map_err(|_| format!("PPM 像素数据不完整: {}", path.display()))?;
            for y in 0..height as usize {
                for x in 0..width as usize {
                    let i = (y * width as usize + x) * 3;
                    image.put(
                        x,
                        y,
                        scale_byte(raw[i], raw[i + 1], raw[i + 2], maxval as u32),
                    );
                }
            }
            return Ok(image);
        }

        for y in 0..height as usize {
            for x in 0..width as usize {
                let r = read_ppm_int(&mut reader).ok_or_else(|| format!("PPM 像素提前结束: {}", path.display()))?;
                let g = read_ppm_int(&mut reader).ok_or_else(|| format!("PPM 像素提前结束: {}", path.display()))?;
                let b = read_ppm_int(&mut reader).ok_or_else(|| format!("PPM 像素提前结束: {}", path.display()))?;
                image.put(x, y, scale_byte(r as u8, g as u8, b as u8, maxval as u32));
            }
        }
        Ok(image)
    }

    pub fn output_ppm(&self, filename: &str) -> std::io::Result<()> {
        self.write_ppm(&output_file(filename))
    }

    pub fn output_bmp(&self, filename: &str) -> std::io::Result<()> {
        self.write_bmp(&output_file(filename))
    }
}

fn write_u16(out: &mut impl Write, value: u16) -> std::io::Result<()> {
    out.write_all(&value.to_le_bytes())
}

fn write_u32(out: &mut impl Write, value: u32) -> std::io::Result<()> {
    out.write_all(&value.to_le_bytes())
}

fn read_ppm_token(reader: &mut impl Read) -> Option<String> {
    let mut token = String::new();
    let mut byte = [0u8; 1];
    loop {
        match reader.read(&mut byte) {
            Ok(0) => return if token.is_empty() { None } else { Some(token) },
            Ok(_) => {
                let ch = byte[0] as char;
                if ch.is_whitespace() {
                    if !token.is_empty() {
                        return Some(token);
                    }
                    continue;
                }
                if ch == '#' {
                    let mut rest = String::new();
                    let mut b = [0u8; 1];
                    while let Ok(n) = reader.read(&mut b) {
                        if n == 0 || b[0] == b'\n' {
                            break;
                        }
                        rest.push(b[0] as char);
                    }
                    continue;
                }
                token.push(ch);
            }
            Err(_) => return None,
        }
    }
}

fn read_ppm_int(reader: &mut impl Read) -> Option<i32> {
    read_ppm_token(reader).and_then(|t| t.parse().ok())
}

fn scale_byte(r: u8, g: u8, b: u8, maxval: u32) -> Color {
    let scale = |v: u8| ((v as u32 * 255) / maxval) as u8;
    Color::new(scale(r), scale(g), scale(b))
}
