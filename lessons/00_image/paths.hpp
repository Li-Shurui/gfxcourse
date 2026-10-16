#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace gfx {

// 资源根目录由 CMake 写入 GFX_ROOT，因此从 build 目录运行也能找到 assets/。
inline std::filesystem::path root_dir() {
#ifdef GFX_ROOT
  return std::filesystem::path{GFX_ROOT};
#else
  return std::filesystem::current_path();
#endif
}

inline std::filesystem::path output_file(const std::string& filename) {
  const auto dir = root_dir() / "output";
  std::filesystem::create_directories(dir);
  return dir / filename;
}

inline bool has_flag(int argc, char** argv, std::string_view flag) {
  for (int i = 1; i < argc; ++i) {
    if (flag == argv[i]) return true;
  }
  return false;
}

}  // namespace gfx
