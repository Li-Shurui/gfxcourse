# 第 7 课习题

## 拓展阅读

- Phong，《Illumination for Computer Generated Pictures》，Communications of the ACM，1975。原始模型用反射向量和视线的夹角。
- Blinn，《Models of Light Reflection for Computer Synthesized Pictures》，SIGGRAPH 1977。半角向量是这篇引入的。
- GAMES101 闫令琪，Lecture 7–8，着色。
- Akenine-Möller 等，《Real-Time Rendering》第 5 章对 Blinn-Phong 的短评。记住它不是能量守恒的：漫反射和高光可以加出大于 1 的颜色。

## 编程题

1. 把半角高光换成经典 Phong：`r = reflect(-l, n)`，用 `max(r · v, 0) ^ shininess`。同一盏灯、同一个球，两张图都存下来。掠射角附近，两者的高光形状不一样。
2. 去掉距离衰减，再把灯移到 `(0, 8, 6)`。记录球的平均亮度变化。加上 `1/r²` 之后再记录一次。写出你认为哪一种更适合这一课的静物。
3. 加第二盏冷色的补光，放在主光的另一侧，强度大约是主光的三分之一。立方体的暗面应该还能看出红色，而不是一片黑。
4. 让着色器返回法线的可视化：`n * 0.5 + 0.5`，再编码成颜色。球应该是连续的渐变，立方体每个面一块纯色。如果立方体的某个面变成了条纹，法线矩阵或插值有问题。

## 思考题

Blinn-Phong 的高光颜色 `ks` 和漫反射 `kd` 彼此独立。金属在真实世界里，高光带着材质的颜色，暗部几乎没有漫反射。这一课的着色器做不到这一点。第 10 课的光栅材质会用一个很粗的系数去模仿，第 12 课的光线追踪才真的去反射。差别在哪里？
