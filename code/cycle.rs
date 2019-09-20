use std::fmt;

// +-> Test.0() -+
// |             |
// +-------------+
struct Test(Box<dyn Fn() -> Test>);

//for those cold lonely nights
//aka "VRRRRRRR" -cpu fan
//also apparently uses a lot of power
fn main() -> ! {
    println!("Hello, world! {}", Test::from(0).0().0().0());
    println!("You should Ctrl+C this program. It is nothing but an infinite loop.");
    let infinite = Test::from(1);
    let _sum: u32 = infinite.map(|_| 1).sum();
    unreachable!()
}

//this is to get the ball rolling
//can't make a closure that refers to the binding it's in:
//let test = Test(Box::new(|| test)); //invalid
impl From<i32> for Test {
    fn from(arg: i32) -> Test {
        Test(Box::new(move || arg.into()))
    }
}

//dummy display impl to quantify the result, since you can't "display" a closure
impl fmt::Display for Test {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        writeln!(f, "test")
    }
}

//why not, the sky's the limit
impl Iterator for Test {
    type Item = Test;

    fn next(&mut self) -> Option<Self::Item> {
        Some(self.0())
    }
}
