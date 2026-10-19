use gfxcourse::bvh::Bvh;
use gfxcourse::image::{Color, Image};
use gfxcourse::pathtracer::render_path;
use gfxcourse::paths::{has_flag, output_file, root_dir};
use gfxcourse::rasterizer::render_raster;
use gfxcourse::scene::load_scene;
use gfxcourse::world::RWorld;
use std::time::Instant;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let preview = has_flag(&args, "--preview");
    // 第一个非选项参数是场景路径（默认 cornell），第二个是输出文件名前缀。
    let positional: Vec<&String> = args.iter().skip(1).filter(|a| !a.starts_with('-')).collect();
    let scene_arg = positional.first().map(|s| s.as_str()).unwrap_or("");
    let prefix = positional.get(1).map(|s| s.as_str()).unwrap_or("");
    let width = if preview { 160 } else { 480 };
    let height = if preview { 120 } else { 360 };
    let samples = if preview { 4 } else { 48 };

    let scene_path = if scene_arg.is_empty() {
        root_dir().join("assets/scenes/cornell.scene")
    } else {
        root_dir().join(scene_arg)
    };
    let scene = match load_scene(&scene_path) {
        Ok(scene) => scene,
        Err(error) => {
            eprintln!("{}", error);
            std::process::exit(1);
        }
    };
    // 不带参数时保持课程默认输出名 16_*.bmp；指定场景后输出 <前缀_><场景名>_*.bmp。
    let tag = if scene_arg.is_empty() {
        "16".to_string()
    } else {
        let stem = std::path::Path::new(scene_arg)
            .file_stem()
            .map(|s| s.to_string_lossy().into_owned())
            .unwrap_or_else(|| "scene".to_string());
        if prefix.is_empty() {
            stem
        } else {
            format!("{}_{}", prefix, stem)
        }
    };

    let mut raster = Image::new(width, height, Color::default());
    render_raster(&scene, &mut raster);

    let world = RWorld::from_scene(&scene);
    let bvh = Bvh::build(&world.triangles);
    let mut traced = Image::new(width, height, Color::default());
    let start = Instant::now();
    render_path(&world, &bvh, &scene.camera, &mut traced, samples);
    let ms = start.elapsed().as_secs_f64() * 1000.0;

    let mut compare = Image::new(width * 2 + 8, height, Color::new(236, 232, 220));
    compare.blit(&raster, 0, 0);
    compare.blit(&traced, width + 8, 0);

    let raster_path = output_file(&format!("{}_raster.bmp", tag));
    let path_path = output_file(&format!("{}_path.bmp", tag));
    let compare_path = output_file(&format!("{}_compare.bmp", tag));
    if raster.write_bmp(&raster_path).is_err()
        || traced.write_bmp(&path_path).is_err()
        || compare.write_bmp(&compare_path).is_err()
    {
        std::process::exit(1);
    }
    println!(
        "渲染完成（{}）。左光栅、右路径追踪：",
        scene_path.file_name().map(|s| s.to_string_lossy()).unwrap_or_default()
    );
    println!("  {}", compare_path.display());
    println!("路径追踪 {:.0} ms，{} spp", ms, samples);
}
