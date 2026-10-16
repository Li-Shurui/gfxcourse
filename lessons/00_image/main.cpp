#include "image.hpp"
#include "log.hpp"

#include <algorithm>
#include <cmath>

int main() {
  constexpr int kSize = 320;
  gfx::Image image(kSize, kSize);

  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      const float u = static_cast<float>(x) / static_cast<float>(kSize - 1);
      const float v = static_cast<float>(y) / static_cast<float>(kSize - 1);
      const float dx = u - 0.5f;
      const float dy = v - 0.5f;
      const float disk = std::sqrt(dx * dx + dy * dy);

      // 背景是从左到右、从上到下的二维渐变。圆盘用到像素中心的距离，这是后面光线的雏形。
      const float shade = std::clamp(1.0f - disk * 2.1f, 0.0f, 1.0f);
      const int r = static_cast<int>(u * 40.0f + shade * 210.0f);
      const int g = static_cast<int>(v * 50.0f + shade * 170.0f);
      const int b = static_cast<int>(90.0f + (1.0f - u) * 40.0f + shade * 80.0f);
      image.put(x, y, {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)});
    }
  }

  const auto bmp = gfx::output_file("00_image.bmp");
  const auto ppm = gfx::output_file("00_image.ppm");
  if (!image.write_bmp(bmp) || !image.write_ppm(ppm)) {
    gfx::say_error("写出图像失败");
    return 1;
  }

  // 自检：左上角不应和圆盘中心一样，文件格式也要能再读回来。
  const gfx::Color corner = image.get(0, 0);
  const gfx::Color center = image.get(kSize / 2, kSize / 2);
  if (corner == center) {
    gfx::say_error("渐变没有画出来");
    return 1;
  }
  const auto loaded = gfx::Image::read_ppm(ppm);
  if (!loaded || (*loaded).get(kSize / 2, kSize / 2) != center) {
    gfx::say_error("PPM 读写不一致");
    return 1;
  }

  gfx::say("第 0 课完成，图像写在：");
  gfx::say("  {}", bmp.string());
  gfx::say("  {}", ppm.string());
  return 0;
}
