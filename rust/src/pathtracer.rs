use crate::bvh::{hit_scene, Bvh};
use crate::camera::Camera;
use crate::color::write_linear;
use crate::image::Image;
use crate::material::MaterialType;
use crate::parallel::{parallel_chunks, worker_count};
use crate::ray::{camera_ray, Hit, Ray};
use crate::rng::{sample_cosine_hemisphere, Rng};
use crate::vec::{
    dot3, fresnel_dielectric, gray, length_squared3, min, normalize3, reflect, refract, Vec3, K_PI,
};
use crate::world::RWorld;

pub fn scene_occluded(world: &RWorld, bvh: &Bvh, ray: &Ray, tmin: f32, tmax: f32) -> bool {
    let mut start = tmin;
    let mut guard = 0;
    while guard < 8 && start < tmax {
        let mut hit = Hit::default();
        if !hit_scene(world, bvh, ray, start, tmax, &mut hit) {
            return false;
        }
        if world.material(hit.material as usize).kind != MaterialType::Dielectric {
            return true;
        }
        start = hit.t + 1e-3;
        guard += 1;
    }
    false
}

pub fn sample_direct(world: &RWorld, bvh: &Bvh, hit: &Hit, albedo: Vec3, rng: &mut Rng) -> Vec3 {
    let lights = &world.scene.area_lights;
    if lights.is_empty() {
        return Vec3::default();
    }
    let count = lights.len() as i32;
    let index = (count - 1).min((rng.uniform() * count as f32) as i32);
    let light = &lights[index as usize];
    let point = light.sample(rng.uniform(), rng.uniform());
    let delta = point - hit.position;
    let dist2 = length_squared3(delta);
    let dist = dist2.sqrt();
    if dist < 1e-4 {
        return Vec3::default();
    }
    let wi = delta / dist;
    let cos_surface = dot3(hit.normal, wi);
    let cos_light = dot3(light.normal(), -wi);
    if cos_surface <= 0.0 || cos_light <= 0.0 {
        return Vec3::default();
    }
    if scene_occluded(
        world,
        bvh,
        &Ray {
            origin: hit.position + hit.normal * 1e-3,
            direction: wi,
        },
        1e-3,
        dist - 2e-3,
    ) {
        return Vec3::default();
    }
    let brdf = albedo * (1.0 / K_PI);
    brdf * light.emission * cos_surface * cos_light * ((count as f32) * light.area()) / dist2
}

pub fn trace_path(world: &RWorld, bvh: &Bvh, mut ray: Ray, rng: &mut Rng) -> Vec3 {
    let mut radiance = Vec3::default();
    let mut throughput = Vec3::new(1.0, 1.0, 1.0);
    let mut count_emission = true;
    const MAX_DEPTH: i32 = 6;
    for bounce in 0..MAX_DEPTH {
        let mut hit = Hit::default();
        if !hit_scene(world, bvh, &ray, 1e-3, 1e30, &mut hit) {
            radiance += throughput * world.miss(&ray);
            break;
        }
        let material = world.material(hit.material as usize);
        let albedo = world.shade_albedo(material, hit.uv);
        if material.kind == MaterialType::Emissive {
            if count_emission {
                radiance += throughput * material.emission;
            }
            break;
        }

        if material.kind == MaterialType::Diffuse {
            radiance += throughput * sample_direct(world, bvh, &hit, albedo, rng);
            let wi = sample_cosine_hemisphere(hit.normal, rng);
            if length_squared3(wi) < 1e-8 {
                break;
            }
            throughput = throughput * albedo;
            count_emission = false;
            ray = Ray {
                origin: hit.position + hit.normal * 1e-3,
                direction: normalize3(wi),
            };
        } else if material.kind == MaterialType::Metal {
            let mut reflected = reflect(normalize3(ray.direction), hit.normal);
            if material.roughness > 0.0 {
                reflected = normalize3(reflected + rng.in_unit_sphere() * material.roughness);
                if dot3(reflected, hit.normal) <= 0.0 {
                    break;
                }
            }
            throughput = throughput * albedo;
            count_emission = true;
            ray = Ray {
                origin: hit.position + reflected * 1e-3,
                direction: reflected,
            };
        } else {
            let incident = normalize3(ray.direction);
            let eta_i = if hit.front { 1.0 } else { material.ior };
            let eta_t = if hit.front { material.ior } else { 1.0 };
            let n = if hit.front { hit.geometric } else { -hit.geometric };
            let cos_i = dot3(-incident, n).min(1.0);
            let mut transmitted = Vec3::default();
            let reflect_amount = if refract(incident, n, eta_i / eta_t, &mut transmitted) {
                fresnel_dielectric(cos_i, eta_i, eta_t)
            } else {
                1.0
            };
            if rng.uniform() < reflect_amount {
                let reflected = reflect(incident, n);
                ray = Ray {
                    origin: hit.position + reflected * 1e-3,
                    direction: reflected,
                };
            } else {
                throughput = throughput * albedo;
                ray = Ray {
                    origin: hit.position + transmitted * 1e-3,
                    direction: transmitted,
                };
            }
            count_emission = true;
        }

        if bounce >= 2 {
            let survive = throughput
                .x
                .max(throughput.y)
                .max(throughput.z)
                .clamp(0.05, 0.95);
            if rng.uniform() > survive {
                break;
            }
            throughput = throughput * (1.0 / survive);
        }
    }
    if !radiance.x.is_finite() || !radiance.y.is_finite() || !radiance.z.is_finite() {
        return Vec3::default();
    }
    min(radiance, gray(16.0))
}

pub fn tone_map(color: Vec3) -> Vec3 {
    let exposed = color * 0.85;
    Vec3::new(
        exposed.x / (1.0 + exposed.x),
        exposed.y / (1.0 + exposed.y),
        exposed.z / (1.0 + exposed.z),
    )
}

pub fn render_path(world: &RWorld, bvh: &Bvh, camera: &Camera, image: &mut Image, samples: i32) {
    let width = image.width();
    let height = image.height();
    if width == 0 || height == 0 {
        return;
    }
    // 每行一个切片，按行段分发给各线程；每个像素仍用自己的种子，结果和单线程完全一致。
    let rows_per_thread = height.div_ceil(worker_count().min(height));
    let mut film = vec![Vec3::default(); width * height];
    parallel_chunks(&mut film, rows_per_thread * width, |chunk, start| {
        let y0 = start / width;
        for (dy, row) in chunk.chunks_mut(width).enumerate() {
            let y = y0 + dy;
            for (x, pixel) in row.iter_mut().enumerate() {
                let mut rng = Rng::new(((y as u64) << 32) ^ (x as u64).wrapping_mul(9781) ^ 0x123456789);
                let mut sum = Vec3::default();
                for _ in 0..samples {
                    let ray = camera_ray(
                        camera,
                        x as f32 + rng.uniform(),
                        y as f32 + rng.uniform(),
                        width,
                        height,
                    );
                    sum += trace_path(world, bvh, ray, &mut rng);
                }
                *pixel = sum * (1.0 / samples as f32);
            }
        }
    });
    for y in 0..height {
        for x in 0..width {
            let color = film[y * width + x];
            write_linear(image, x, y, tone_map(color));
        }
    }
}
