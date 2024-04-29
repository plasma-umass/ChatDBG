fn fact(n: f32) -> f32 {
  let mut x = 1.0;
  for i in 0..n as i32 {
      x *= i as f32;
  }
  assert!(x != 0.0);
  x
}

#[chatdbg::main]
fn main() {
  println!("{}", fact(100.0));
}
