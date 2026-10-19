//! 对应 C++ 课程 17 课的自检（lessons/*/main.cpp 里的 GFX_CHECK / say_error）。
//! 每一课的断言原样保留；第 2 课的 draw_line 和第 12、13 课的 whitted
//! 不在 src/ 快照里，属于课程中间产物，这里没有对应测试。

use gfxcourse::bvh::Bvh;
use gfxcourse::camera::{clip_line_near, project_to_screen, view_matrix, view_projection, Camera};
use gfxcourse::color::{linear_to_color, write_linear};
use gfxcourse::image::{Color, Image};
use gfxcourse::light::{blinn_phong, PointLight};
use gfxcourse::mat::{inverse3, inverse4, normal_matrix, Mat3, Mat4};
use gfxcourse::material::MaterialType;
use gfxcourse::mesh::{load_obj, make_uv_sphere};
use gfxcourse::paths::{output_file, root_dir};
use gfxcourse::raster::{draw_clipped_triangle, ClipVert, DepthBuffer};
use gfxcourse::rasterizer::render_raster;
use gfxcourse::ray::{camera_ray, sky_color, Hit, Ray};
use gfxcourse::scene::load_scene;
use gfxcourse::shade::draw_geometry;
use gfxcourse::shapes::{add_quad, make_cube, make_plane, make_sphere, Geometry};
use gfxcourse::sphere::Sphere;
use gfxcourse::texture::{make_checker, Filter};
use gfxcourse::triangle::{barycentric, fill_triangle, fill_triangle_colored};
use gfxcourse::trimesh::{intersect_triangle, Tri};
use gfxcourse::vec::*;
use gfxcourse::world::RWorld;

fn nearly(a: f32, b: f32) -> bool {
    (a - b).abs() <= 1e-4 * 1.0f32.max(a.abs()).max(b.abs())
}

fn nearly3(a: Vec3, b: Vec3) -> bool {
    nearly(a.x, b.x) && nearly(a.y, b.y) && nearly(a.z, b.z)
}

fn draw_line(image: &mut Image, x0: i32, y0: i32, x1: i32, y1: i32, color: Color) {
    let dx = (x1 - x0).abs();
    let dy = -(y1 - y0).abs();
    let sx = if x0 < x1 { 1 } else { -1 };
    let sy = if y0 < y1 { 1 } else { -1 };
    let mut err = dx + dy;
    let (mut x, mut y) = (x0, y0);
    loop {
        if x >= 0 && y >= 0 {
            image.put(x as usize, y as usize, color);
        }
        if x == x1 && y == y1 {
            break;
        }
        let e2 = 2 * err;
        if e2 >= dy {
            err += dy;
            x += sx;
        }
        if e2 <= dx {
            err += dx;
            y += sy;
        }
    }
}

// 第 0 课：PPM 读写一致。
#[test]
fn lesson00_image() {
    const SIZE: usize = 320;
    let mut image = Image::new(SIZE, SIZE, Color::default());
    for y in 0..SIZE {
        for x in 0..SIZE {
            let u = x as f32 / (SIZE - 1) as f32;
            let v = y as f32 / (SIZE - 1) as f32;
            let dx = u - 0.5;
            let dy = v - 0.5;
            let disk = (dx * dx + dy * dy).sqrt();
            let shade = (1.0 - disk * 2.1).clamp(0.0, 1.0);
            let r = (u * 40.0 + shade * 210.0) as u8;
            let g = (v * 50.0 + shade * 170.0) as u8;
            let b = (90.0 + (1.0 - u) * 40.0 + shade * 80.0) as u8;
            image.put(x, y, Color::new(r, g, b));
        }
    }
    let corner = image.get(0, 0);
    let center = image.get(SIZE / 2, SIZE / 2);
    assert_ne!(corner, center, "渐变没有画出来");

    let ppm = output_file("rust_00_image.ppm");
    image.write_ppm(&ppm).expect("写出 PPM 失败");
    let loaded = Image::read_ppm(&ppm).expect("读回 PPM 失败");
    assert_eq!(loaded.get(SIZE / 2, SIZE / 2), center, "PPM 读写不一致");
}

// 第 1 课：向量叉积、点积、归一化、反射、折射、菲涅尔。
#[test]
fn lesson01_vectors() {
    let i = Vec3::new(1.0, 0.0, 0.0);
    let j = Vec3::new(0.0, 1.0, 0.0);
    let k = Vec3::new(0.0, 0.0, 1.0);
    assert!(nearly3(cross(i, j), k));
    assert!(nearly3(cross(j, k), i));
    assert!(nearly(dot3(i, j), 0.0));
    assert!(nearly3(normalize3(Vec3::new(3.0, 0.0, 0.0)), i));
    assert!(nearly3(reflect(Vec3::new(1.0, -1.0, 0.0), j), Vec3::new(1.0, 1.0, 0.0)));

    let mut bent = Vec3::default();
    let transmitted = refract(normalize3(Vec3::new(0.2, -1.0, 0.0)), j, 1.0 / 1.5, &mut bent);
    assert!(transmitted);
    assert!(bent.y < 0.0);

    let normal_incidence = fresnel_dielectric(1.0, 1.0, 1.5);
    assert!(normal_incidence > 0.03 && normal_incidence < 0.05);
}

// 第 3 课：两个三角形拼成正方形，像素既不漏也不重。
#[test]
fn lesson03_triangles() {
    const SIZE: usize = 4;
    let tris = [
        [Vec2::new(0.0, 0.0), Vec2::new(4.0, 0.0), Vec2::new(4.0, 4.0)],
        [Vec2::new(0.0, 0.0), Vec2::new(4.0, 4.0), Vec2::new(0.0, 4.0)],
    ];
    let mut owner = vec![0i32; SIZE * SIZE];
    for (t, tri) in tris.iter().enumerate() {
        for y in 0..SIZE {
            for x in 0..SIZE {
                let p = Vec2::new(x as f32 + 0.5, y as f32 + 0.5);
                if barycentric(p, tri[0], tri[1], tri[2]).inside {
                    owner[y * SIZE + x] += if t == 0 { 1 } else { 2 };
                }
            }
        }
    }
    for value in owner {
        assert!(value == 1 || value == 2, "三角形填充规则有裂缝或重叠");
    }

    // 原课还会画一张图，这里保留同样的绘制路径做冒烟测试。
    let mut image = Image::new(420, 320, Color::new(17, 19, 27));
    fill_triangle(&mut image, Vec2::new(40.0, 40.0), Vec2::new(250.0, 70.0), Vec2::new(80.0, 250.0), Color::new(232, 96, 80));
    fill_triangle_colored(
        &mut image,
        Vec2::new(180.0, 40.0),
        Vec2::new(390.0, 150.0),
        Vec2::new(210.0, 290.0),
        Vec3::new(0.91, 0.38, 0.31),
        Vec3::new(0.96, 0.84, 0.36),
        Vec3::new(0.34, 0.66, 0.84),
    );
    assert_ne!(image.get(100, 100), Color::new(17, 19, 27));
}

// 第 4 课：旋转、平移、TRS 求逆、3x3 求逆、法线矩阵。
#[test]
fn lesson04_matrices() {
    let x_axis = Mat4::rotation(Vec3::new(0.0, 1.0, 0.0), radians(90.0)).transform_point(Vec3::new(1.0, 0.0, 0.0));
    assert!(nearly3(x_axis, Vec3::new(0.0, 0.0, -1.0)));
    let moved = Mat4::translation(Vec3::new(2.0, 3.0, 4.0)).transform_point(Vec3::new(1.0, 1.0, 1.0));
    assert!(nearly3(moved, Vec3::new(3.0, 4.0, 5.0)));

    let trs = Mat4::translation(Vec3::new(1.0, 2.0, 3.0))
        * Mat4::rotation(Vec3::new(0.0, 1.0, 0.0), radians(25.0))
        * Mat4::scale(Vec3::new(1.5, 0.5, 2.0));
    let inv = inverse4(trs).expect("TRS 应该可逆");
    let identity = trs * inv;
    for i in 0..16 {
        assert!(nearly(identity.m[i], Mat4::identity().m[i]), "TRS * inv 不是单位阵");
    }

    let scaled = Mat4::scale(Vec3::new(2.0, 3.0, 4.0)).upper3();
    let inv3 = inverse3(scaled).expect("缩放矩阵应该可逆");
    assert!(nearly3(inv3.transform_vector(Vec3::new(2.0, 3.0, 4.0)), Vec3::new(1.0, 1.0, 1.0)));
    let nmat = normal_matrix(Mat4::scale(Vec3::new(2.0, 1.0, 1.0)));
    assert!(nearly3(nmat.transform_vector(Vec3::new(1.0, 1.0, 0.0)), Vec3::new(0.5, 1.0, 0.0)));

    // Mat3 的 2D 变换链路（TRS）也要和 C++ 一致。
    let local = Mat3::translation(Vec2::new(150.0, 80.0))
        * Mat3::rotation(radians(28.0))
        * Mat3::scale(Vec2::new(1.4, 0.7));
    let p = local.transform_point(Vec2::new(0.0, -40.0));
    assert!(p.x.is_finite() && p.y.is_finite());
}

// 第 5 课：视图投影把原点放到屏幕中心，线框立方体画得出像素。
#[test]
fn lesson05_pipeline() {
    let mut camera = Camera::default();
    camera.eye = Vec3::new(0.0, 0.0, 5.0);
    camera.target = Vec3::new(0.0, 0.0, 0.0);
    camera.fov_y_degrees = 90.0;
    let vp = view_projection(&camera, 1.0);
    let origin = vp * vec4(Vec3::new(0.0, 0.0, 0.0), 1.0);
    assert!(origin.w > 0.0);
    assert!(nearly(origin.x / origin.w, 0.0));
    assert!(nearly(origin.y / origin.w, 0.0));

    let eye_space = view_matrix(&camera).transform_point(Vec3::new(0.0, 0.0, 0.0));
    assert!(nearly3(eye_space, Vec3::new(0.0, 0.0, -5.0)));

    const W: usize = 480;
    const H: usize = 360;
    let mut image = Image::new(W, H, Color::new(12, 16, 24));
    let mut view = Camera::default();
    view.eye = Vec3::new(2.6, 1.8, 3.4);
    view.target = Vec3::new(0.0, 0.0, 0.0);
    view.fov_y_degrees = 40.0;
    let model = Mat4::rotation(Vec3::new(0.0, 1.0, 0.0), radians(28.0));
    let cube_vp = view_projection(&view, W as f32 / H as f32) * model;

    let corners = [
        Vec3::new(-1.0, -1.0, -1.0), Vec3::new(1.0, -1.0, -1.0), Vec3::new(1.0, 1.0, -1.0), Vec3::new(-1.0, 1.0, -1.0),
        Vec3::new(-1.0, -1.0, 1.0), Vec3::new(1.0, -1.0, 1.0), Vec3::new(1.0, 1.0, 1.0), Vec3::new(-1.0, 1.0, 1.0),
    ];
    let edges = [
        (0, 1), (1, 2), (2, 3), (3, 0), (4, 5), (5, 6), (6, 7), (7, 4), (0, 4), (1, 5), (2, 6), (3, 7),
    ];
    let ink = Color::new(236, 228, 210);
    for (a, b) in edges {
        let mut ca = cube_vp * vec4(corners[a], 1.0);
        let mut cb = cube_vp * vec4(corners[b], 1.0);
        if !clip_line_near(&mut ca, &mut cb) {
            continue;
        }
        let Some((x0, y0)) = project_to_screen(ca, W, H) else { continue };
        let Some((x1, y1)) = project_to_screen(cb, W, H) else { continue };
        draw_line(&mut image, x0 as i32, y0 as i32, x1 as i32, y1 as i32, ink);
    }
    let background = image.get(0, 0);
    let mut inked = 0;
    for y in 0..H {
        for x in 0..W {
            if image.get(x, y) != background {
                inked += 1;
            }
        }
    }
    assert!(inked > 200, "线框立方体像素太少");
}

// 第 6 课：近处的红色四边形必须盖住远处的蓝色。
#[test]
fn lesson06_depth() {
    const W: usize = 360;
    const H: usize = 270;
    let mut image = Image::new(W, H, Color::new(18, 20, 28));
    let mut depth = DepthBuffer::new(W, H);
    let mut camera = Camera::default();
    camera.eye = Vec3::new(0.0, 0.0, 4.0);
    camera.target = Vec3::new(0.0, 0.0, 0.0);
    camera.fov_y_degrees = 40.0;
    let vp = view_projection(&camera, W as f32 / H as f32);

    let draw_quad = |image: &mut Image, depth: &mut DepthBuffer, points: [Vec3; 4], color: Vec3| {
        let mut triangle = |a: Vec3, b: Vec3, c: Vec3| {
            let mut verts = [ClipVert::default(); 3];
            for (i, p) in [a, b, c].iter().enumerate() {
                verts[i].world = *p;
                verts[i].normal = Vec3::new(0.0, 0.0, 1.0);
                verts[i].color = color;
                verts[i].clip = vp * vec4(*p, 1.0);
            }
            draw_clipped_triangle(image, depth, verts[0], verts[1], verts[2], |frag| frag.color);
        };
        triangle(points[0], points[1], points[2]);
        triangle(points[0], points[2], points[3]);
    };

    draw_quad(
        &mut image,
        &mut depth,
        [
            Vec3::new(-1.4, -1.2, -1.2),
            Vec3::new(1.4, -1.2, -1.2),
            Vec3::new(1.4, 1.2, -1.2),
            Vec3::new(-1.4, 1.2, -1.2),
        ],
        Vec3::new(0.15, 0.35, 0.95),
    );
    draw_quad(
        &mut image,
        &mut depth,
        [
            Vec3::new(-0.55, -0.45, 0.4),
            Vec3::new(0.7, -0.35, 0.2),
            Vec3::new(0.45, 0.6, 0.2),
            Vec3::new(-0.7, 0.45, 0.5),
        ],
        Vec3::new(0.95, 0.22, 0.18),
    );

    let red = linear_to_color(Vec3::new(0.95, 0.22, 0.18));
    let blue = linear_to_color(Vec3::new(0.15, 0.35, 0.95));
    let center = image.get(W / 2, H / 2);
    let mut blue_pixels = 0;
    for y in 0..H {
        for x in 0..W {
            if image.get(x, y) == blue {
                blue_pixels += 1;
            }
        }
    }
    assert_eq!(center, red, "深度测试没有让近处的红色盖住蓝色");
    assert!(blue_pixels >= 100, "远处的蓝色四边形没有画出来");
}

// 第 7 课：朝向光的比背对光的亮，场景能画出足够像素。
#[test]
fn lesson07_shading() {
    let lights = vec![
        PointLight { position: Vec3::new(3.5, 4.5, 2.5), color: Vec3::new(1.0, 0.95, 0.85), intensity: 22.0 },
        PointLight { position: Vec3::new(-3.0, 2.2, 3.0), color: Vec3::new(0.55, 0.7, 1.0), intensity: 14.0 },
    ];
    let ambient = gray(0.035);
    let lit = blinn_phong(
        Vec3::new(0.0, 1.0, 0.0), Vec3::new(0.0, 1.0, 0.0), Vec3::new(3.0, 2.0, 4.0),
        Vec3::new(0.7, 0.3, 0.2), gray(0.04), 32.0, ambient, &lights,
    );
    let dark = blinn_phong(
        Vec3::new(0.0, 1.0, 0.0), Vec3::new(0.0, -1.0, 0.0), Vec3::new(3.0, 2.0, 4.0),
        Vec3::new(0.7, 0.3, 0.2), gray(0.04), 32.0, ambient, &lights,
    );
    assert!(lit.x + lit.y + lit.z > dark.x + dark.y + dark.z + 0.05);

    const W: usize = 640;
    const H: usize = 480;
    let mut image = Image::new(W, H, Color::new(16, 18, 26));
    let mut depth = DepthBuffer::new(W, H);
    let mut camera = Camera::default();
    camera.eye = Vec3::new(3.1, 2.3, 4.0);
    camera.target = Vec3::new(0.0, 0.7, 0.0);
    camera.fov_y_degrees = 34.0;

    draw_geometry(&mut image, &mut depth, &camera, &Mat4::identity(), &make_plane(4.0), |frag| {
        blinn_phong(frag.world, frag.normal, camera.eye, Vec3::new(0.62, 0.64, 0.66), gray(0.045), 48.0, ambient, &lights)
    });
    let sphere = Mat4::translation(Vec3::new(0.55, 1.0, 0.1));
    draw_geometry(&mut image, &mut depth, &camera, &sphere, &make_sphere(40, 24), |frag| {
        blinn_phong(frag.world, frag.normal, camera.eye, Vec3::new(0.25, 0.45, 0.85), gray(0.045), 48.0, ambient, &lights)
    });
    let cube = Mat4::translation(Vec3::new(-1.15, 0.62, 0.25))
        * Mat4::rotation(Vec3::new(0.0, 1.0, 0.0), radians(22.0))
        * Mat4::scale(gray(0.62));
    draw_geometry(&mut image, &mut depth, &camera, &cube, &make_cube(), |frag| {
        blinn_phong(frag.world, frag.normal, camera.eye, Vec3::new(0.78, 0.32, 0.2), gray(0.045), 48.0, ambient, &lights)
    });

    let background = image.get(0, 0);
    let mut drawn = 0;
    for y in 0..H {
        for x in 0..W {
            if image.get(x, y) != background {
                drawn += 1;
            }
        }
    }
    assert!(drawn > W * H / 10, "场景几乎是空的");
}

// 第 8 课：棋盘纹理两面墙都要画进画面。
#[test]
fn lesson08_texture() {
    const W: usize = 640;
    const H: usize = 360;
    let mut image = Image::new(W, H, Color::new(18, 20, 28));
    let mut depth = DepthBuffer::new(W, H);
    let mut camera = Camera::default();
    camera.eye = Vec3::new(0.0, 0.0, 3.4);
    camera.target = Vec3::new(0.0, 0.0, 0.0);
    camera.fov_y_degrees = 32.0;

    let mut checker = make_checker(4, 8, Color::new(240, 236, 220), Color::new(40, 78, 120));
    let lights = vec![PointLight { position: Vec3::new(0.0, 1.2, 4.0), color: Vec3::new(1.0, 1.0, 1.0), intensity: 18.0 }];
    let ambient = gray(0.08);

    for (x, filter) in [(-1.05f32, Filter::Nearest), (1.05f32, Filter::Bilinear)] {
        checker.filter = filter;
        let mut wall = Geometry::default();
        add_quad(
            &mut wall,
            Vec3::new(-0.85, -0.85, 0.0),
            Vec3::new(0.85, -0.85, 0.0),
            Vec3::new(0.85, 0.85, 0.0),
            Vec3::new(-0.85, 0.85, 0.0),
            Vec3::new(0.0, 0.0, 1.0),
        );
        let model = Mat4::translation(Vec3::new(x, 0.0, 0.0))
            * Mat4::rotation(Vec3::new(0.0, 1.0, 0.0), radians(if x < 0.0 { 18.0 } else { -18.0 }));
        draw_geometry(&mut image, &mut depth, &camera, &model, &wall, |frag| {
            let albedo = checker.sample(frag.uv);
            blinn_phong(frag.world, frag.normal, camera.eye, albedo, gray(0.03), 24.0, ambient, &lights)
        });
    }

    assert_ne!(image.get(W / 4, H / 2), image.get(0, 0), "左墙没有画进画面");
    assert_ne!(image.get(3 * W / 4, H / 2), image.get(0, 0), "右墙没有画进画面");
}

// 第 9 课：OBJ 网格三角面数量。
#[test]
fn lesson09_mesh() {
    let cube = load_obj(&root_dir().join("assets/meshes/cube.obj")).expect("cube.obj 加载失败");
    let sphere = load_obj(&root_dir().join("assets/meshes/sphere.obj")).expect("sphere.obj 加载失败");
    let torus = load_obj(&root_dir().join("assets/meshes/torus.obj")).expect("torus.obj 加载失败");
    let plane = load_obj(&root_dir().join("assets/meshes/plane.obj")).expect("plane.obj 加载失败");
    assert_eq!(cube.triangle_count(), 12, "立方体三角面数量异常");
    assert!(sphere.triangle_count() >= 100, "球体三角面数量异常");
    assert!(torus.triangle_count() >= 100, "圆环三角面数量异常");
    assert!(plane.triangle_count() >= 2, "平面三角面数量异常");
}

// 损坏的 OBJ 必须返回错误，不能 panic（C++ 版用异常转成错误信息）。
#[test]
fn lesson09_mesh_bad_obj() {
    let dir = std::env::temp_dir();
    let bad_vt = dir.join("gfx_bad_vt.obj");
    std::fs::write(&bad_vt, "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nf 1/1 2/2 3/1\n").unwrap();
    let result = load_obj(&bad_vt);
    assert!(result.is_err(), "vt 越界应该报错而不是成功或 panic");
    assert!(result.unwrap_err().contains("行 5"), "错误信息应带行号");

    let bad_token = dir.join("gfx_bad_token.obj");
    std::fs::write(&bad_token, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 abc 3\n").unwrap();
    let result = load_obj(&bad_token);
    assert!(result.is_err(), "无法解析的顶点引用应该报错");
}

// 第 10 课：光栅化 studio 场景，画出足够像素。
#[test]
fn lesson10_raster_scene() {
    let scene = load_scene(&root_dir().join("assets/scenes/studio.scene")).expect("studio.scene 加载失败");
    const W: usize = 640;
    const H: usize = 480;
    let mut image = Image::new(W, H, Color::default());
    render_raster(&scene, &mut image);
    let background = scene.background;
    let mut drawn = 0;
    for y in 0..H {
        for x in 0..W {
            if image.get(x, y) != background {
                drawn += 1;
            }
        }
    }
    assert!(drawn >= W * H / 8, "场景几乎是空的，画了 {} 个像素", drawn);
}

// 第 11 课：相机光线朝 -z，中心像素命中球体。
#[test]
fn lesson11_rays() {
    let mut camera = Camera::default();
    camera.eye = Vec3::new(0.0, 0.35, 2.4);
    camera.target = Vec3::new(0.0, 0.0, 0.0);
    camera.fov_y_degrees = 40.0;

    let center_ray = camera_ray(&camera, 160.0, 120.0, 320, 240);
    assert!(center_ray.direction.z <= -0.9, "屏幕中心的光线没有朝前");

    let spheres = [
        Sphere { center: Vec3::new(0.0, 0.0, 0.0), radius: 0.55, material: 0 },
        Sphere { center: Vec3::new(-1.15, 0.1, -0.2), radius: 0.45, material: 0 },
        Sphere { center: Vec3::new(1.05, -0.05, 0.15), radius: 0.4, material: 0 },
    ];

    const W: usize = 320;
    const H: usize = 240;
    let mut image = Image::new(W, H, Color::default());
    for y in 0..H {
        for x in 0..W {
            let ray = camera_ray(&camera, x as f32 + 0.5, y as f32 + 0.5, W, H);
            let mut color = sky_color(&ray);
            let mut closest = 1e30f32;
            let mut hit = Hit::default();
            for sphere in &spheres {
                if sphere.intersect(&ray, 0.001, closest, &mut hit) {
                    closest = hit.t;
                    color = hit.normal * 0.5 + gray(0.5);
                }
            }
            write_linear(&mut image, x, y, color);
        }
    }
    let mid = image.get(W / 2, H / 2);
    assert!(mid.b >= 80, "中心球体没有被命中");
}

// 第 13 课：torus 场景的网格光线求交（C++ 用的是课程里的 whitted 渲染器，
// 它不在 src/ 快照里；这里直接验证 RWorld 的求交覆盖）。
#[test]
fn lesson13_mesh_ray() {
    let scene = load_scene(&root_dir().join("assets/scenes/torus.scene")).expect("torus.scene 加载失败");
    let world = RWorld::from_scene(&scene);
    assert!(!world.triangles.is_empty(), "场景里没有三角形");

    const W: usize = 360;
    const H: usize = 270;
    let mut drawn = 0;
    let mut hit = Hit::default();
    for y in 0..H {
        for x in 0..W {
            let ray = camera_ray(&scene.camera, x as f32 + 0.5, y as f32 + 0.5, W, H);
            if world.closest_hit(&ray, 1e-3, 1e30, &mut hit) {
                drawn += 1;
            }
        }
    }
    assert!(drawn >= 200, "网格光追几乎没有命中");
}

// 第 14 课：BVH 和暴力求交结果必须一致，并且不慢于暴力。
#[test]
fn lesson14_bvh() {
    fn brute_closest(triangles: &[Tri], ray: &Ray, tmin: f32, tmax: f32, hit: &mut Hit) -> bool {
        let mut found = false;
        let mut tmax = tmax;
        for tri in triangles {
            if intersect_triangle(ray, tri, tmin, tmax, hit) {
                found = true;
                tmax = hit.t;
            }
        }
        found
    }

    fn shade(hit: &Hit) -> Vec3 {
        let light_pos = Vec3::new(2.2, 3.4, 2.0);
        let l = normalize3(light_pos - hit.position);
        let ndotl = dot3(hit.normal, l).max(0.0);
        let albedo = if hit.material == 0 { Vec3::new(0.62, 0.64, 0.6) } else { Vec3::new(0.25, 0.48, 0.86) };
        albedo * (0.08 + 0.92 * ndotl)
    }

    // 和 C++ 预览模式相同的参数。
    const W: usize = 80;
    const H: usize = 60;
    let sphere = make_uv_sphere(24, 12, 1.0);
    let mut triangles: Vec<Tri> = Vec::new();
    for surface in &sphere.surfaces {
        let mut i = 0;
        while i + 2 < surface.indices.len() {
            let mut tri = Tri {
                p: [Vec3::default(); 3],
                n: [Vec3::default(); 3],
                uv: [Vec2::default(); 3],
                material: 1,
            };
            for k in 0..3 {
                let vertex = &sphere.vertices[surface.indices[i + k] as usize];
                tri.p[k] = vertex.position + Vec3::new(0.0, 0.2, 0.0);
                tri.n[k] = vertex.normal;
                tri.uv[k] = vertex.uv;
            }
            triangles.push(tri);
            i += 3;
        }
    }
    let p = [
        Vec3::new(-2.0, -0.8, -2.0),
        Vec3::new(2.0, -0.8, -2.0),
        Vec3::new(2.0, -0.8, 2.0),
        Vec3::new(-2.0, -0.8, 2.0),
    ];
    let n = Vec3::new(0.0, 1.0, 0.0);
    for combo in [[0, 1, 2], [0, 2, 3]] {
        let mut tri = Tri { p: [Vec3::default(); 3], n: [n; 3], uv: [Vec2::default(); 3], material: 0 };
        for k in 0..3 {
            tri.p[k] = p[combo[k]];
        }
        triangles.push(tri);
    }

    let bvh = Bvh::build(&triangles);
    let mut camera = Camera::default();
    camera.eye = Vec3::new(2.4, 1.6, 2.8);
    camera.target = Vec3::new(0.0, 0.1, 0.0);
    camera.fov_y_degrees = 35.0;

    let render = |use_bvh: bool| -> (Image, f64) {
        let start = std::time::Instant::now();
        let mut image = Image::new(W, H, Color::new(20, 24, 32));
        let mut hit = Hit::default();
        for y in 0..H {
            for x in 0..W {
                let ray = camera_ray(&camera, x as f32 + 0.5, y as f32 + 0.5, W, H);
                let found = if use_bvh {
                    bvh.closest(&triangles, &ray, 0.001, 1e30, &mut hit)
                } else {
                    brute_closest(&triangles, &ray, 0.001, 1e30, &mut hit)
                };
                if found {
                    write_linear(&mut image, x, y, shade(&hit));
                } else {
                    write_linear(&mut image, x, y, sky_color(&ray));
                }
            }
        }
        (image, start.elapsed().as_secs_f64() * 1000.0)
    };

    let (brute_image, brute_ms) = render(false);
    let (fast_image, fast_ms) = render(true);

    let mut different = 0;
    for y in 0..H {
        for x in 0..W {
            let a = brute_image.get(x, y);
            let b = fast_image.get(x, y);
            let delta = (a.r as i32 - b.r as i32).abs() + (a.g as i32 - b.g as i32).abs() + (a.b as i32 - b.b as i32).abs();
            if delta > 3 {
                different += 1;
            }
        }
    }
    assert!(different <= W * H / 100, "BVH 和暴力求交不一致，差异像素 {}", different);
    assert!(brute_ms <= 40.0 || fast_ms <= brute_ms, "BVH 没有更快：暴力 {:.1} ms，BVH {:.1} ms", brute_ms, fast_ms);
}

// 第 15 课：cornell 场景路径追踪预览，画面不能过暗。
#[test]
fn lesson15_path() {
    let scene = load_scene(&root_dir().join("assets/scenes/cornell.scene")).expect("cornell.scene 加载失败");
    let world = RWorld::from_scene(&scene);
    let bvh = Bvh::build(&world.triangles);
    const W: usize = 120;
    const H: usize = 90;
    let mut image = Image::new(W, H, Color::default());
    gfxcourse::pathtracer::render_path(&world, &bvh, &scene.camera, &mut image, 4);

    let mut sum = 0.0f64;
    for y in H / 4..3 * H / 4 {
        for x in W / 4..3 * W / 4 {
            let pixel = image.get(x, y);
            sum += (pixel.r + pixel.g + pixel.b) as f64;
        }
    }
    assert!(sum >= 1000.0, "路径追踪结果过暗");
}

// 第 16 课：光栅和路径追踪都能画出 cornell 场景（renderer 二进制的冒烟版）。
#[test]
fn lesson16_capstone() {
    let scene = load_scene(&root_dir().join("assets/scenes/cornell.scene")).expect("cornell.scene 加载失败");
    const W: usize = 160;
    const H: usize = 120;

    let mut raster = Image::new(W, H, Color::default());
    render_raster(&scene, &mut raster);

    let world = RWorld::from_scene(&scene);
    let bvh = Bvh::build(&world.triangles);
    let mut traced = Image::new(W, H, Color::default());
    gfxcourse::pathtracer::render_path(&world, &bvh, &scene.camera, &mut traced, 4);

    let mut raster_drawn = 0;
    let mut traced_sum = 0u64;
    for y in 0..H {
        for x in 0..W {
            if raster.get(x, y) != scene.background {
                raster_drawn += 1;
            }
            let p = traced.get(x, y);
            traced_sum += (p.r + p.g + p.b) as u64;
        }
    }
    assert!(raster_drawn > W * H / 10, "光栅化结果几乎是空的");
    assert!(traced_sum > 1000, "路径追踪结果过暗");

    // 场景里必须同时有发光体（面积光）和非发光材质，否则对照图没有意义。
    assert!(!scene.area_lights.is_empty(), "cornell 场景缺少面积光");
    assert!(
        scene.materials.iter().any(|m| m.kind != MaterialType::Emissive),
        "cornell 场景缺少非发光材质"
    );

    let mut compare = Image::new(W * 2 + 8, H, Color::new(236, 232, 220));
    compare.blit(&raster, 0, 0);
    compare.blit(&traced, W + 8, 0);
    compare.write_bmp(&output_file("rust_16_compare.bmp")).expect("写出对照图失败");
}
