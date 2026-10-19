use crate::shapes::{make_sphere, Geometry};
use crate::vec::{normalize3, Vec2, Vec3};
use std::collections::HashMap;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

#[derive(Clone, Debug)]
pub struct Vertex {
    pub position: Vec3,
    pub normal: Vec3,
    pub uv: Vec2,
}

#[derive(Clone, Debug)]
pub struct Surface {
    pub material: String,
    pub indices: Vec<u32>,
}

#[derive(Clone, Debug, Default)]
pub struct Mesh {
    pub vertices: Vec<Vertex>,
    pub surfaces: Vec<Surface>,
}

impl Mesh {
    pub fn triangle_count(&self) -> usize {
        self.surfaces.iter().map(|s| s.indices.len() / 3).sum()
    }
}

pub fn mesh_from_geometry(geometry: &Geometry, radius: f32) -> Mesh {
    let mut mesh = Mesh::default();
    for i in 0..geometry.positions.len() {
        mesh.vertices.push(Vertex {
            position: geometry.positions[i] * radius,
            normal: geometry.normals[i],
            uv: if i < geometry.uvs.len() { geometry.uvs[i] } else { Vec2::default() },
        });
    }
    mesh.surfaces.push(Surface {
        material: String::new(),
        indices: geometry.indices.clone(),
    });
    mesh
}

pub fn make_uv_sphere(slices: usize, stacks: usize, radius: f32) -> Mesh {
    mesh_from_geometry(&make_sphere(slices, stacks), radius)
}

/// 解析 OBJ 面片的一个角："v"、"v/vt"、"v//vn"、"v/vt/vn"。
/// 解析失败返回错误，而不是静默当成 0（C++ 版用异常报错）。
fn parse_corner(token: &str) -> Result<(i32, i32, i32), String> {
    let invalid = || format!("无法解析顶点引用: {}", token);
    let mut parts = token.split('/');
    let parse = |part: Option<&str>| -> Result<Option<i32>, String> {
        match part {
            None | Some("") => Ok(None),
            Some(text) => text.parse::<i32>().map(Some).map_err(|_| invalid()),
        }
    };
    let v = parse(parts.next())?.ok_or_else(invalid)?;
    let vt = parse(parts.next())?.unwrap_or(0);
    let vn = parse(parts.next())?.unwrap_or(0);
    Ok((v, vt, vn))
}

fn surface_for<'a>(mesh: &'a mut Mesh, name: &str) -> &'a mut Surface {
    if mesh.surfaces.last().map(|s| s.material.as_str()) != Some(name) {
        mesh.surfaces.push(Surface {
            material: name.to_string(),
            indices: Vec::new(),
        });
    }
    mesh.surfaces.last_mut().unwrap()
}

pub fn load_obj(path: &Path) -> Result<Mesh, String> {
    let file = File::open(path).map_err(|e| format!("无法打开 OBJ: {}: {}", path.display(), e))?;
    let reader = BufReader::new(file);

    let mut positions: Vec<Vec3> = Vec::new();
    let mut uvs: Vec<Vec2> = Vec::new();
    let mut normals: Vec<Vec3> = Vec::new();
    let mut mesh = Mesh::default();
    let mut authored_normal: Vec<bool> = Vec::new();
    let mut unique: HashMap<(i32, i32, i32), u32> = HashMap::new();
    let mut material = String::new();
    let mut any_missing_normal = false;

    let fix_index = |index: i32, count: i32| -> i32 {
        if index > 0 {
            index - 1
        } else if index < 0 {
            count + index
        } else {
            -1
        }
    };

    let add_corner = |mesh: &mut Mesh,
                          position_index: i32,
                          uv_index: i32,
                          normal_index: i32,
                          positions: &Vec<Vec3>,
                          uvs: &Vec<Vec2>,
                          normals: &Vec<Vec3>,
                          authored_normal: &mut Vec<bool>,
                          unique: &mut HashMap<(i32, i32, i32), u32>,
                          any_missing_normal: &mut bool|
     -> Result<u32, String> {
        let key = (position_index, uv_index, normal_index);
        if let Some(&id) = unique.get(&key) {
            return Ok(id);
        }
        // 位置下标调用方已经检查过；uv / 法线下标在这里校验，越界报错而不是 panic。
        let mut vertex = Vertex {
            position: *positions
                .get(position_index as usize)
                .ok_or("位置下标越界")?,
            normal: Vec3::default(),
            uv: Vec2::default(),
        };
        if uv_index >= 0 {
            vertex.uv = *uvs.get(uv_index as usize).ok_or("UV 下标越界")?;
        }
        let has_normal = normal_index >= 0;
        if has_normal {
            vertex.normal = *normals.get(normal_index as usize).ok_or("法线下标越界")?;
        } else {
            *any_missing_normal = true;
        }
        let id = mesh.vertices.len() as u32;
        mesh.vertices.push(vertex);
        authored_normal.push(has_normal);
        unique.insert(key, id);
        Ok(id)
    };

    for (line_number, line) in reader.lines().enumerate() {
        let line_number = line_number + 1;
        let line = line.map_err(|e| format!("OBJ 读取失败: {}", e))?;
        let line = line.trim_end_matches('\r');
        let line = if let Some(comment) = line.find('#') {
            &line[..comment]
        } else {
            line
        };
        let mut stream = line.split_whitespace();
        let tag = match stream.next() {
            Some(t) => t,
            None => continue,
        };
        match tag {
            "v" => {
                let x: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let y: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let z: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                positions.push(Vec3::new(x, y, z));
            }
            "vt" => {
                let u: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let v: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                uvs.push(Vec2::new(u, v));
            }
            "vn" => {
                let x: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let y: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let z: f32 = stream.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                normals.push(Vec3::new(x, y, z));
            }
            "usemtl" => {
                material = stream.next().unwrap_or("").to_string();
            }
            "f" => {
                let mut corners = Vec::new();
                for token in stream {
                    let (v, vt, vn) = parse_corner(token)
                        .map_err(|e| format!("OBJ 解析失败，行 {}: {}", line_number, e))?;
                    let pi = fix_index(v, positions.len() as i32);
                    let ti = fix_index(vt, uvs.len() as i32);
                    let ni = fix_index(vn, normals.len() as i32);
                    if pi < 0 || pi >= positions.len() as i32 {
                        return Err(format!("OBJ 顶点下标越界，行 {}", line_number));
                    }
                    corners.push(
                        add_corner(
                            &mut mesh,
                            pi,
                            ti,
                            ni,
                            &positions,
                            &uvs,
                            &normals,
                            &mut authored_normal,
                            &mut unique,
                            &mut any_missing_normal,
                        )
                        .map_err(|e| format!("OBJ 解析失败，行 {}: {}", line_number, e))?,
                    );
                }
                if corners.len() < 3 {
                    continue;
                }
                let surface = surface_for(&mut mesh, &material);
                for i in 1..corners.len() - 1 {
                    surface.indices.push(corners[0]);
                    surface.indices.push(corners[i]);
                    surface.indices.push(corners[i + 1]);
                }
            }
            _ => {}
        }
    }

    if any_missing_normal {
        let mut accumulated = vec![Vec3::default(); mesh.vertices.len()];
        for surface in &mesh.surfaces {
            for i in (0..surface.indices.len()).step_by(3) {
                if i + 2 >= surface.indices.len() {
                    break;
                }
                let a = mesh.vertices[surface.indices[i] as usize].position;
                let b = mesh.vertices[surface.indices[i + 1] as usize].position;
                let c = mesh.vertices[surface.indices[i + 2] as usize].position;
                let face = crate::vec::cross(b - a, c - a);
                for k in 0..3 {
                    let index = surface.indices[i + k] as usize;
                    if !authored_normal[index] {
                        accumulated[index] += face;
                    }
                }
            }
        }
        for i in 0..mesh.vertices.len() {
            if !authored_normal[i] {
                mesh.vertices[i].normal = normalize3(accumulated[i]);
            }
        }
    }

    if mesh.surfaces.is_empty() {
        return Err(format!("OBJ 里没有面: {}", path.display()));
    }
    Ok(mesh)
}
