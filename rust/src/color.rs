use crate::image::{Color, Image};
use crate::vec::Vec3;

#[inline]
pub fn linear_to_srgb(channel: f32) -> f32 {
    let channel = channel.clamp(0.0, 1.0);
    if channel <= 0.0031308 {
        12.92 * channel
    } else {
        1.055 * channel.powf(1.0 / 2.4) - 0.055
    }
}

#[inline]
pub fn srgb_to_linear(channel: f32) -> f32 {
    let channel = channel.clamp(0.0, 1.0);
    if channel <= 0.04045 {
        channel / 12.92
    } else {
        ((channel + 0.055) / 1.055).powf(2.4)
    }
}

#[inline]
pub fn srgb_to_linear3(color: Color) -> Vec3 {
    Vec3::new(
        srgb_to_linear(color.r as f32 / 255.0),
        srgb_to_linear(color.g as f32 / 255.0),
        srgb_to_linear(color.b as f32 / 255.0),
    )
}

#[inline]
pub fn linear_to_color(linear: Vec3) -> Color {
    let byte = |channel: f32| {
        let encoded = linear_to_srgb(channel);
        (encoded * 255.0).round() as u8
    };
    Color::new(byte(linear.x), byte(linear.y), byte(linear.z))
}

#[inline]
pub fn write_linear(image: &mut Image, x: usize, y: usize, linear: Vec3) {
    image.put(x, y, linear_to_color(linear));
}
