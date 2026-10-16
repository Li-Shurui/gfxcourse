# 第 10 课　场景与光栅器

前面的课把能力一块一块接上。这一课把它们收成一个场景文件和一个渲染入口。相机、灯、材质、网格和球都写在文本里。`render_raster` 读入场景，画出 `output/10_raster_scene.bmp`。

## 你应该看到什么

一张静物。地面是重复的砖或格子。左侧一块陶土色的立方体，右侧一颗偏蓝的球，近处一个金黄色的环面，环面附近还有一颗较小的、带高光的球。灯在前上方，物体在地面上有明暗，但没有彼此投下的影子。光栅器不会去问「这段光被谁挡住」。

环面应当能看出中间的洞。它是金属材质，在这一课里表现为更亮的高光和偏黄的底色，不会真的映出立方体。那是第 12 课的事。

程序统计画上的像素。少得离谱就失败，用来抓住「场景没加载」或「相机看向了空处」。它不保证构图好看。

## 场景文件

语法是一行一条命令，`#` 后面是注释。材质必须写在引用它的网格之前。路径相对于仓库根目录。

```text
camera ex ey ez  tx ty tz  ux uy uz  fov
ambient r g b
background r g b
sky 0|1
point_light x y z  r g b  intensity
area_light cx cy cz  ux uy uz  vx vy vz  er eg eb
material 名字 diffuse r g b  纹理路径|none
material 名字 metal r g b  roughness
material 名字 dielectric r g b  ior
material 名字 emissive r g b
mesh 材质名  tx ty tz  sx sy sz  绕y角度  模型路径
sphere 材质名  x y z  半径
```

`background` 的三个数是 0 到 255。`sky` 为 1 时，光线追踪的未命中走天空梯度；光栅器只用 `background` 清屏。`area_light` 这一课的光栅器不用，路径追踪用。

`mesh` 的变换是平移、绕 y 轴的旋转、缩放，矩阵顺序 `T * Ry * S`。OBJ 里的 `usemtl` 如果在场景里有同名材质，就覆盖网格那一行的默认材质。引用了不存在的名字是错误，加载失败，不会悄悄涂成白色。

示例在 `assets/scenes/studio.scene`。

## 光栅器怎么处理材质

漫反射用反照率当 `kd`，高光是一小块灰色。金属没有真正的反射光线，所以 `shade_fragment` 把一部分反照率留在漫反射里，高光使用金属的颜色，光泽度随粗糙度变。玻璃同样保留一部分漫反射，并给一个较硬的白高光。这样静物里的金子和玻璃球才看得见。它们看起来像塑料，这是诚实的：局部光照模型画不出周围的物体。

自发光材质直接返回 `emission`，不乘灯光。

球在光栅器里用 `make_uv_sphere` 细分后再走三角形管线。场景里的 `sphere` 和 `mesh` 对光栅器来说最后都是三角形。对第 11 课以后的光线追踪来说，球仍是解析曲面。

纹理在 `sample` 时从 sRGB 解码。材质的 `albedo` 三元组按线性颜色解释，场景文件里写 0.7 就是线性的 0.7，不会再解码一次。

## 跟着代码走

`scene.hpp` 的 `load_scene` 把文件收成 token，再按命令填 `Scene`。`rasterizer.hpp` 的 `render_raster` 清屏、画每个网格、细分每个球。着色函数在同一个文件的前半。

`main.cpp` 只加载 `studio.scene` 并写出图片。改构图时改场景文件，不必改代码。

## 运行

```powershell
.\build\lesson10.exe
```

把 `studio.scene` 里金环面的 `roughness` 改成 0.8 再运行。高光变宽，颜色更像磨砂金属。光栅器仍然不会在环面上映出那颗蓝球。

## 这一课留下的限制

没有阴影，没有反射，没有折射，没有间接光。从下一课起换一种做法：从相机发射线，问它先碰到谁。场景文件还会继续用。
