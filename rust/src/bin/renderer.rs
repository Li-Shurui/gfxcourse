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
    let width = if preview { 160 } else { 480 };
    let height = if preview { 120 } else { 360 };
    let samples = if preview { 4 } else { 48 };

    let scene = match load_scene(&root_dir().join("assets/scenes/cornell.scene")) {
        Ok(scene) => scene,
        Err(error) => {
            eprintln!("{}", error);
            std::process::exit(1);
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

    let raster_path = output_file("16_raster.bmp");
    let path_path = output_file("16_path.bmp");
    let compare_path = output_file("16_compare.bmp");
    if raster.write_bmp(&raster_path).is_err()
        || traced.write_bmp(&path_path).is_err()
        || compare.write_bmp(&compare_path).is_err()
    {
        std::process::exit(1);
    }
    println!("第 16 课完成。左光栅、右路径追踪：");
    println!("  {}", compare_path.display());
    println!("路径追踪 {:.0} ms，{} spp", ms, samples);
}
