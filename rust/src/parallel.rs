use std::thread;

pub fn worker_count() -> usize {
    thread::available_parallelism().map(|n| n.get()).unwrap_or(4)
}

/// 把 data 按 chunk_len 切成互斥的几段，每段交给一个线程跑 f(段, 段的起始下标)。
/// chunks_mut 保证任意时刻每段只有一个 &mut，全程安全代码。
pub fn parallel_chunks<T: Send>(data: &mut [T], chunk_len: usize, f: impl Fn(&mut [T], usize) + Send + Sync) {
    let chunk_len = chunk_len.max(1);
    if data.len() <= chunk_len {
        f(data, 0);
        return;
    }
    let f = &f;
    thread::scope(|scope| {
        for (i, chunk) in data.chunks_mut(chunk_len).enumerate() {
            scope.spawn(move || f(chunk, i * chunk_len));
        }
    });
}
