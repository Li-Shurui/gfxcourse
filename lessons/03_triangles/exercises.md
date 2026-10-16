# 第 3 课习题

## 拓展阅读

- Scratchapixel，《Rasterization: a Practical Implementation》，重心坐标与 edge function 两节。对照本课 `edge` 的符号和 y 朝下的约定。
- Juan Pineda，《A Parallel Algorithm for Polygon Rasterization》，SIGGRAPH 1988。边函数可以在像素之间增量更新。本课为了读得清楚，每个像素都重算。
- Microsoft Direct3D 文档里的 Rasterization Rules。top-left 不是本课自创的，硬件管线用同一条规则保证相邻三角形不重叠。

## 编程题

1. 构造两个共边三角形，在 4×4 的图上打印每个像素被写的次数。改坏 `is_top_left` 的水平边条件，看双写出现在顶边还是底边。再改回来。
2. 用重心坐标判断点是否在三角形内，不要先交换绕序。负面积时三个权重会变成负数。写出你的「内部」条件，使它和现在的 `inside` 在随机点上一致。
3. 给 `fill_triangle` 加一个纹理坐标参数，这一课可以先用一张你在 `main` 里手写的 2×2 颜色表。权重用来采样。第 8 课会把这件事做成真正的纹理。
4. 只画三角形的三条边，不填内部。边用第 2 课的 `draw_line`。和填充结果叠在一起，看线上的像素是否都被填充盖住。顶边上会有差别，把差别和 top-left 规则对上。

## 思考题

包围盒遍历在又细又斜的三角形上会做很多落空的边函数测试。增量边函数能把复杂度从「包围盒面积」降到什么？你现在值不值得为第 3 课的图去写它？
