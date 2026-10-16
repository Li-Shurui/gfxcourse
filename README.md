# 软件渲染器与路径追踪

这是一门用 C++23 从空白画布写到路径追踪的图形学课。每一课都是一个能单独运行的程序：看懂这一课的代码，改一两处，再打开它写出的图片。十七课结束时，你手里会有同一个场景的两张图——左边是光栅化，右边是路径追踪。

课程不依赖窗口库、图像库或数学库。图画进内存里的像素数组，再写成 BMP 和 PPM。构建只用 CMake 和编译器自带的标准库。

## 你需要先会什么

会写 C++，知道类、引用、模板和 `CMake` 大概是什么。向量的加法、点积、叉积如果还生疏，第 1 课会重新推一遍，但线性代数的记号要看得下去。这门课不教语言，也不从「什么是像素」再往前讲。

## 怎么构建

需要 CMake 3.20 或更新，以及支持 C++23 的编译器。在仓库根目录执行。

Windows，MinGW：

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j
ctest --test-dir build --output-on-failure
.\build\lesson00.exe
```

Windows，Visual Studio 生成器会把可执行文件放到 `build\Release\`（或多配置目录）下面，把上面的路径换成那个目录即可。

macOS / Linux：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/lesson00
```

默认是 Release。图片不写在当前工作目录，而写在仓库里的 `output/`。测试也不依赖你从哪个目录启动。

第 14、15、16 课的测试带 `--preview`，分辨率和采样数都比较低，用来确认程序没坏。自己看图时不要加这个参数。

## 怎么学

按课号往下走。每一课的目录里有两份文档：

- `README.md` 是这一课的教程：画面里应该出现什么、公式为什么长这样、代码从哪个函数走进去。
- `exercises.md` 是拓展阅读和编程题。先把正课跑通，再做题。

每一课新增的头文件只被这一课和后面的课看见。第 5 课包含不了第 6 课的深度缓冲，这是故意的：你看到的接口，就是写到这里时渲染器真正拥有的能力。头文件名字全仓库唯一，后面的课是在加能力，不是在偷偷换掉前面的实现。`lessons/00_image/console.cpp` 会链进每一课，只为了让 Windows 控制台用 UTF-8 打出中文。

建议的节奏是一课一个晚上：读教程，从该课的 `main.cpp` 跟进去，运行，对照 `output/` 里的图，再做一两道编程题。

## 课表

| 课 | 目录 | 你将得到 |
| --- | --- | --- |
| 0 | `lessons/00_image` | 帧缓冲，BMP / PPM |
| 1 | `lessons/01_vectors` | 向量、反射、折射、菲涅尔 |
| 2 | `lessons/02_lines` | Bresenham 直线 |
| 3 | `lessons/03_triangles` | 重心坐标与 top-left 填充 |
| 4 | `lessons/04_matrices` | 矩阵、变换、法线矩阵 |
| 5 | `lessons/05_pipeline` | 相机、透视、线框 |
| 6 | `lessons/06_depth` | 裁剪、深度、透视校正插值 |
| 7 | `lessons/07_shading` | Blinn-Phong |
| 8 | `lessons/08_texture` | 最近邻与双线性，sRGB |
| 9 | `lessons/09_mesh` | OBJ 网格 |
| 10 | `lessons/10_raster_scene` | 场景文件和完整光栅器 |
| 11 | `lessons/11_rays` | 光线与球 |
| 12 | `lessons/12_materials` | Whitted 光追：金属、玻璃、阴影 |
| 13 | `lessons/13_mesh_ray` | 三角形相交 |
| 14 | `lessons/14_bvh` | 层次包围盒 |
| 15 | `lessons/15_path` | 路径追踪 |
| 16 | `lessons/16_capstone` | 同一场景，光栅与光追并排 |

## 全课共用的约定

坐标系是右手系，y 朝上，相机默认看向 −z。图片的原点在左上角，y 朝下。这两种 y 只在投影到屏幕时相遇，第 5 课会把符号写清楚。

矩阵按列存储，`operator()(row, col)` 用行、列下标访问。向量是列向量，变换写成 v′ = Mv。一个点先缩放、再旋转、再平移时，矩阵要写成 T·R·S，因为乘法从右边先作用到向量上。

光照在线性空间里算。写进图片之前做真正的 sRGB 编码，不是简单的 γ = 2.2。读纹理时再解码回线性。第 6 课引入这个转换，第 8 课解释为什么跳过它会让棋盘格发灰。

颜色的 `Vec3` 三个分量是 r、g、b。灰色用 `gray(v)`。`Vec3{v}` 不是灰色，它是 `(v, 0, 0)`。

## 素材

`assets/meshes/` 里有立方体、平面、球、环面和康奈尔盒的 OBJ。`assets/textures/` 是两张 PPM 棋盘格。`assets/scenes/` 是三份场景：第 10 课的静物、第 13 课的环面、第 15 和 16 课的康奈尔盒。

这些文件由 `tools/make_assets.cpp` 生成，已经放进仓库，不需要先跑一遍才能上课。想改网格密度或重写场景时：

```powershell
cmake --build build --target make_assets
.\build\make_assets.exe
```

场景语法在第 10 课的教程里。

## 测试在查什么

`ctest` 跑的就是每一课的可执行文件。它们会检查一些不可能靠肉眼稳定发现的事：PPM 能否读回、重心坐标是否把相邻三角形的共享边正好填一次、近处的红方块是否盖住远处的蓝方块、BVH 和暴力遍历是否画出同一张图、路径追踪的画面是否全黑。通过测试不等于画面好看。第 8、10、12、15、16 课要自己打开图片看。

## 目录

```text
lessons/     十七课。每一课一个目录，含 main.cpp、新的头文件、教程和习题
assets/      网格、纹理、场景
tools/       生成素材的程序
output/      运行后出现，不进版本库
```
