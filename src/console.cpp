// 让 Windows 控制台按 UTF-8 输出中文。这个翻译单元会被之后的每一课链接进去。
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {

struct Utf8Console {
  Utf8Console() { ::SetConsoleOutputCP(CP_UTF8); }
};

const Utf8Console kUtf8Console;

}  // namespace
#endif
