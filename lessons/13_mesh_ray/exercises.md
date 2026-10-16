# 第 13 课习题

## 拓展阅读

- Möller 与 Trumbore，《Fast, Minimum Storage Ray-Triangle Intersection》，Journal of Graphics Tools，1997。对照 `intersect_triangle` 里的 `det`、`u`、`v`、`t`。文章很短。
- Scratchapixel，《Ray Tracing: Rendering a Triangle》。先看几何推导，再看为什么不必存储平面方程。
- Shirley，《Ray Tracing: The Next Week》里关于三角形网格的章节。他先把模型读进来，再谈包围盒。你可以按这个顺序预习第 14 课。

## 编程题

1. 只保留一个三角形，射线从正面和背面各打一次。打印 `front` 和着色法线。背面入射时法线应该翻过来。
2. 统计一次渲染里 `intersect_triangle` 被调用的次数，除以像素数。考虑反射和阴影之后，每个像素远远不止一次。把这个数留到第 14 课对比。
3. 给三角形求交加一个「双面 / 单面」开关。单面时 `det` 的符号不对就丢弃。环面如果只有一半亮，是这个开关和法线朝向不一致。
4. 手写两个共面、共边的三角形，射线打在共享边上。允许两个都算命中，但 `t` 必须几乎相等。top-left 是光栅化的规则，光线追踪在边上可以命中任意一个，只要不漏。

## 思考题

平滑法线能让低面数的环面看起来圆，轮廓却仍然是折线。为什么插值法线改不了轮廓？轮廓由哪一个法线决定，着色由哪一个决定？
