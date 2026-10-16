# 第 2 课习题

## 拓展阅读

- Bresenham，《Algorithm for computer control of a digital plotter》，IBM Systems Journal，1965。原文很短，对照本课的误差项看他如何避免除法。
- Scratchapixel，《Rasterization: a Practical Implementation》里画线的一节。
- Foley 等，《Computer Graphics: Principles and Practice》里关于 DDA 与 Bresenham 的对比。DDA 每步做一次浮点加法，Bresenham 把同样的决策变成整数。

## 编程题

1. 画一根从 `(0, 0)` 到 `(100, 1)` 的线，把经过的像素打出来。你应该看到 y 只跳变一两次，而且跳变前后 x 是连续的。再画反向的同一根线，像素集合应该相同。
2. 实现虚线：每画 `on` 个像素就空 `off` 个像素。用它重画星形。确认端点仍然被画上，否则虚线的相位会在每根线的起点漂移。
3. 给直线加一个像素的宽度，方法任选一种：沿法线方向再画一根，或者把点画成 2×2。写清你的做法在端点处会不会重复涂色。这一课没有混合，重复涂同一颜色是允许的。
4. 实现 Xiaolin Wu 的抗锯齿直线，把端点的覆盖率写成颜色的亮度。和 Bresenham 的结果并排放。这一题可以只做斜率在 0 到 1 的情况。

## 思考题

为什么游戏里的线框调试视图直到今天还常能看见 Bresenham，而最终画面几乎不再用它画物体？它丢了哪种信息，是后来的三角形光栅化必须重新找回来的？
