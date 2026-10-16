# 第 14 课习题

## 拓展阅读

- Shirley，《Ray Tracing: The Next Week》中 Bounding Volume Hierarchies 一章。他的划分和本课的中点分裂同一类。
- Wald，《On fast Construction of SAH-based Bounding Volume Hierarchies》，2007。先知道 SAH 在优化什么，不必实现 binning。
- Scratchapixel，《Introduction to Acceleration Structures》。
- Akenine-Möller 等，《Real-Time Rendering》里关于 BVH 与 kd-tree 的短节。本课选 BVH，是因为一套轴对齐盒子对动态和静态场景都好写。

## 编程题

1. 打印树的深度和每片叶子的三角形数。中点分裂在这颗均匀的球上应该比较平衡。再构造一个「一个很大的三角形加上一千个挤在角落里的小三角形」的场景，看深度变成什么样。
2. 故意把某个内部节点的子节点下标写成 0，看测试如何失败。确认失败来自像素比较，而不是崩溃。然后改回。
3. 记录每次命中真正调用三角形测试的次数，和包围盒测试的次数。和你在第 13 课记的「每像素三角形测试数」放在一起。
4. 实现按三角形数量一分为二的中位数划分，替换中点划分。图像必须仍然一致。在第 2 题那种不均匀的场景上比较时间。

## 思考题

包围盒是轴对齐的。把一整组三角形旋转 45°，盒子会变松，里面多出很多空的角落。松了以后，正确性变不变？速度变不变？有没有必要在建树前把坐标轴转到这组三角形自己的方向上？
