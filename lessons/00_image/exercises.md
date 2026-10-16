# 第 0 课习题

## 拓展阅读

- Netpbm 的 PPM 说明（P3 与 P6）：对照 `Image::write_ppm` 和 `Image::read_ppm`，看魔数、空白和最大通道值各占哪一段。
- Microsoft 的 BMP 文件结构，BITMAPFILEHEADER 与 BITMAPINFOHEADER：对照 `write_bmp` 里那 54 个字节。注意 `biHeight` 为正时行序是自下而上。
- Shirley，《Fundamentals of Computer Graphics》第 3 章开头关于 framebuffer 的几页。只需要建立「图像是数组」这件事。

## 编程题

1. 把实心圆盘改成圆环。距离在某个区间内才加亮，区间外保持背景渐变。运行后中心应该露出渐变，而不是一块亮斑。
2. 在 `image.hpp` 里实现 `fill_circle(Image&, Vec2 center, float radius, Color)`。这一课还没有 `Vec2` 的话，用两个 `int` 也行；做完第 1 课后把参数改成向量。用第 0 课的 `main` 画三个互不包含的圆，确认边角没有被涂掉。
3. 让 `write_ppm` 可选地写 P3。用文本编辑器改一个像素的数字，再用 `read_ppm` 读回来，看改动落在你期望的坐标上。借此确认行序和通道顺序。
4. 写一个只读 24 位无压缩 BMP 的 `read_bmp`。用第 0 课写出的文件做往返。先不要支持别的位深。

## 思考题

为什么帧缓冲用 8 位整数，而后面的光照要用浮点？如果全程用 0…255 做乘加，暗部会先损失什么？
