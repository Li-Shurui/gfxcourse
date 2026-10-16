# 第 5 课习题

## 拓展阅读

- GAMES101 闫令琪，Lecture 4，观测变换与透视投影。对照本课 `view_matrix` 第三行的负号。
- Scratchapixel，《The Perspective and Orthographic Projection Matrix》。本课是 OpenGL 的 z 映射（近 −1，远 +1），不是 Direct3D 的 0 到 1。抄公式时先看清是哪一种。
- Shirley，《Fundamentals of Computer Graphics》第 7 章，视锥体。

## 编程题

1. 在 `main.cpp` 里加一个正交投影函数，用左右、上下、近、远六个参数。让线框立方体在正交和透视之间切换。正交下，前后两条边在屏幕上应该一样长。
2. 把视野角从 40° 改到 80°，再改到 15°。写出画面上立方体变大还是变小，并指出 `perspective` 里哪一个元素随视野角变化。
3. 构造一条一端在相机背后的线段，打印裁剪后的两个端点。背后的那个端点应该被拉到近平面上，而不是被删掉后留下半截错线。两端都在背后时，`clip_line_near` 返回 false。
4. 实现一个「看向点，但 up 和 forward 几乎平行」的相机。`normalize(cross(forward, up))` 会接近零。你的程序现在会怎样？给 `view_matrix` 加一个保护，并写一个测试。

## 思考题

NDC 的 z 已经能区分远近，为什么第 6 课的深度缓冲却不用裁剪空间的 z，而用除以 w 之后的窗口深度？和透视除法的非线性有关。先猜，再读下一课。
