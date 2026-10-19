use std::path::{Path, PathBuf};

pub fn root_dir() -> PathBuf {
    // 优先运行期环境变量，其次编译期 GFX_ROOT（和 C++ 课程一致）。
    if let Ok(root) = std::env::var("GFX_ROOT") {
        if !root.is_empty() {
            return PathBuf::from(root);
        }
    }
    if let Some(root) = option_env!("GFX_ROOT") {
        return PathBuf::from(root);
    }
    // 从当前目录向上找带 assets/ 的目录，这样在仓库任何子目录（包括 rust/）下都能运行。
    if let Ok(cwd) = std::env::current_dir() {
        for dir in cwd.ancestors() {
            if dir.join("assets").is_dir() {
                return dir.to_path_buf();
            }
        }
        return cwd;
    }
    PathBuf::from(".")
}

pub fn output_file(filename: &str) -> PathBuf {
    let dir = root_dir().join("output");
    let _ = std::fs::create_dir_all(&dir);
    dir.join(filename)
}

pub fn has_flag(args: &[String], flag: &str) -> bool {
    args.iter().skip(1).any(|a| a == flag)
}

pub fn resolve_asset(text: &str) -> PathBuf {
    let path = Path::new(text);
    if path.is_relative() {
        root_dir().join(path)
    } else {
        path.to_path_buf()
    }
}
