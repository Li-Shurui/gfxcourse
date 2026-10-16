#pragma once

#include <format>
#include <iostream>
#include <utility>

namespace gfx {

// 这套 MinGW 的 std::print 还缺终端符号，课程统一用 std::format 写到 iostream。
template <class... Args>
void say(std::format_string<Args...> fmt, Args&&... args) {
  std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

template <class... Args>
void say_error(std::format_string<Args...> fmt, Args&&... args) {
  std::cerr << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

}  // namespace gfx
