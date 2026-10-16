#pragma once

#include "paths.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace gfx {

struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;

  constexpr bool operator==(const Color&) const = default;
};

// 原点在左上角、y 向下的 RGB 帧缓冲。软件渲染器先把图存进这块内存，再写成文件。
class Image {
 public:
  Image() = default;
  Image(int width, int height, Color fill = {}) { reset(width, height, fill); }

  void reset(int width, int height, Color fill = {}) {
    width_ = std::max(width, 0);
    height_ = std::max(height, 0);
    pixels_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 3, 0);
    clear(fill);
  }

  int width() const { return width_; }
  int height() const { return height_; }
  bool empty() const { return width_ == 0 || height_ == 0; }

  void clear(Color fill) {
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) put(x, y, fill);
    }
  }

  void put(int x, int y, Color color) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    const std::size_t i = index(x, y);
    pixels_[i] = color.r;
    pixels_[i + 1] = color.g;
    pixels_[i + 2] = color.b;
  }

  Color get(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return {};
    const std::size_t i = index(x, y);
    return {pixels_[i], pixels_[i + 1], pixels_[i + 2]};
  }

  // C++23 多参数下标：image[x, y]
  Color operator[](int x, int y) const { return get(x, y); }

  void fill_rect(int x, int y, int w, int h, Color color) {
    for (int yy = y; yy < y + h; ++yy) {
      for (int xx = x; xx < x + w; ++xx) put(xx, yy, color);
    }
  }

  // 把 src 拷到 this 的 (dst_x, dst_y)。越界部分丢掉。
  void blit(const Image& src, int dst_x, int dst_y) {
    for (int y = 0; y < src.height(); ++y) {
      for (int x = 0; x < src.width(); ++x) put(dst_x + x, dst_y + y, src.get(x, y));
    }
  }

  bool write_ppm(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    out.write(reinterpret_cast<const char*>(pixels_.data()),
              static_cast<std::streamsize>(pixels_.size()));
    return static_cast<bool>(out);
  }

  bool write_bmp(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    const int row_stride = (width_ * 3 + 3) & ~3;
    const std::uint32_t pixel_bytes =
        static_cast<std::uint32_t>(row_stride) * static_cast<std::uint32_t>(height_);
    const std::uint32_t file_size = 54u + pixel_bytes;

    write_u16(out, 0x4D42);
    write_u32(out, file_size);
    write_u16(out, 0);
    write_u16(out, 0);
    write_u32(out, 54);
    write_u32(out, 40);
    write_u32(out, static_cast<std::uint32_t>(width_));
    write_u32(out, static_cast<std::uint32_t>(height_));
    write_u16(out, 1);
    write_u16(out, 24);
    write_u32(out, 0);
    write_u32(out, pixel_bytes);
    write_u32(out, 2835);
    write_u32(out, 2835);
    write_u32(out, 0);
    write_u32(out, 0);

    const std::vector<char> pad(static_cast<std::size_t>(row_stride - width_ * 3), 0);
    // BMP 自下而上。帧缓冲的 y = 0 在上面，所以倒着写。
    for (int y = height_ - 1; y >= 0; --y) {
      for (int x = 0; x < width_; ++x) {
        const Color c = get(x, y);
        const char bgr[3] = {static_cast<char>(c.b), static_cast<char>(c.g), static_cast<char>(c.r)};
        out.write(bgr, 3);
      }
      if (!pad.empty()) out.write(pad.data(), static_cast<std::streamsize>(pad.size()));
    }
    return static_cast<bool>(out);
  }

  static std::expected<Image, std::string> read_ppm(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::unexpected("无法打开 PPM: " + path.string());

    std::string magic;
    in >> magic;
    if (magic != "P3" && magic != "P6") return std::unexpected("只支持 P3/P6 PPM: " + path.string());

    const int width = read_ppm_int(in);
    const int height = read_ppm_int(in);
    const int maxval = read_ppm_int(in);
    if (width <= 0 || height <= 0 || maxval <= 0 || maxval > 255) {
      return std::unexpected("PPM 头无效: " + path.string());
    }

    Image image(width, height);
    if (magic == "P6") {
      in.get();  // 吃掉 maxval 后面的那一个空白
      std::vector<char> raw(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3);
      in.read(raw.data(), static_cast<std::streamsize>(raw.size()));
      if (in.gcount() != static_cast<std::streamsize>(raw.size())) {
        return std::unexpected("PPM 像素数据不完整: " + path.string());
      }
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                 static_cast<std::size_t>(x)) *
                                3;
          image.put(x, y, scale_byte(raw[i], raw[i + 1], raw[i + 2], maxval));
        }
      }
      return image;
    }

    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const int r = read_ppm_int(in);
        const int g = read_ppm_int(in);
        const int b = read_ppm_int(in);
        if (r < 0 || g < 0 || b < 0) return std::unexpected("PPM 像素提前结束: " + path.string());
        image.put(x, y, scale_byte(static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), maxval));
      }
    }
    return image;
  }

 private:
  int width_ = 0;
  int height_ = 0;
  std::vector<std::uint8_t> pixels_;

  std::size_t index(int x, int y) const {
    return (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)) * 3;
  }

  static void write_u16(std::ostream& out, std::uint16_t value) {
    const char bytes[2] = {static_cast<char>(value & 255), static_cast<char>((value >> 8) & 255)};
    out.write(bytes, 2);
  }

  static void write_u32(std::ostream& out, std::uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 255),
        static_cast<char>((value >> 8) & 255),
        static_cast<char>((value >> 16) & 255),
        static_cast<char>((value >> 24) & 255),
    };
    out.write(bytes, 4);
  }

  static int read_ppm_int(std::istream& in) {
    char ch = 0;
    while (in.get(ch)) {
      if (std::isspace(static_cast<unsigned char>(ch))) continue;
      if (ch == '#') {
        std::string ignored;
        std::getline(in, ignored);
        continue;
      }
      in.unget();
      int value = -1;
      in >> value;
      return value;
    }
    return -1;
  }

  static Color scale_byte(char r, char g, char b, int maxval) {
    const auto scale = [maxval](unsigned char v) {
      return static_cast<std::uint8_t>((static_cast<int>(v) * 255) / maxval);
    };
    return {scale(static_cast<unsigned char>(r)), scale(static_cast<unsigned char>(g)),
            scale(static_cast<unsigned char>(b))};
  }
};

}  // namespace gfx
