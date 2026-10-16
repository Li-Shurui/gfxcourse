#pragma once

#include "image.hpp"
#include "vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gfx {

inline float linear_to_srgb(float channel) {
  channel = std::clamp(channel, 0.0f, 1.0f);
  if (channel <= 0.0031308f) return 12.92f * channel;
  return 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f;
}

inline float srgb_to_linear(float channel) {
  channel = std::clamp(channel, 0.0f, 1.0f);
  if (channel <= 0.04045f) return channel / 12.92f;
  return std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

inline Vec3 srgb_to_linear(Color color) {
  return {srgb_to_linear(color.r / 255.0f), srgb_to_linear(color.g / 255.0f),
          srgb_to_linear(color.b / 255.0f)};
}

inline Color linear_to_color(Vec3 linear) {
  const auto byte = [](float channel) {
    const float encoded = linear_to_srgb(channel);
    return static_cast<std::uint8_t>(std::lround(encoded * 255.0f));
  };
  return {byte(linear.x), byte(linear.y), byte(linear.z)};
}

inline void write_linear(Image& image, int x, int y, Vec3 linear) {
  image.put(x, y, linear_to_color(linear));
}

}  // namespace gfx
