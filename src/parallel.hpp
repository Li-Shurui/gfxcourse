#pragma once

#include <algorithm>
#include <thread>
#include <vector>

namespace gfx {

template <class Fn>
void parallel_rows(int height, Fn&& fn) {
  if (height <= 0) return;
  int workers = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
  if (workers > height) workers = height;
  if (workers <= 1) {
    fn(0, height);
    return;
  }
  const int chunk = (height + workers - 1) / workers;
  std::vector<std::jthread> threads;
  threads.reserve(static_cast<std::size_t>(workers));
  for (int i = 0; i < workers; ++i) {
    const int y0 = i * chunk;
    const int y1 = std::min(height, y0 + chunk);
    if (y0 >= y1) break;
    threads.emplace_back([&, y0, y1] { fn(y0, y1); });
  }
}

}  // namespace gfx
