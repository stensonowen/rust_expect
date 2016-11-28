//Test for basic __builtin_expect_ usage

//Compile without stdlib: don't have to link to test
//Pretty sure to compile a *binary* w/o stdlib you need nightly Rust
#![no_std]
#![no_main]
#![feature(lang_items, start)]
#[lang = "eh_personality"] extern fn eh_personality() {}
#[lang = "panic_fmt"] fn panic_fmt() -> ! { loop {} }

const ITERS: f64 = 10_000_000_000.;

//#[no_mangle]
// generics require mangling because compilation generates all used forms
// if we mangle, we get a warning and can only use the function w/ one type for T	
// if we no_mangle, we have to search for function names containing (not equalling) a string
#[inline]
fn __builtin_expect_<T: Eq>(expr: T, expect: T) -> bool {
	expr == expect
}

#[inline]
fn __blank_function_<T: Eq>(expr: T, expect: T) -> bool {
	expr == expect
}

#[no_mangle] 
pub extern fn main(argc: i32, _argv: *const *const u8) -> i32 {
    let fn_to_test = builtin_test;
    //let fn_to_test = control2;

    fn_to_test(argc as f64) as i32
}

fn control(mut y: f64) -> f64 {
    let mut count: f64 = 0.0;
    while count < ITERS {
        count += 1.0;
        if __blank_function_(count > 5.0,true) {
            y += 1.0;
        } else {
            y += 2.0;
        }
    }
    y
}

fn builtin_test(mut y: f64) -> f64 {
    let mut count: f64 = 0.0;
    while count < ITERS {
        count += 1.0;
        if __builtin_expect_(count > 5.0,true) {
            y += 1.0;
        } else {
            y += 2.0;
        }
    }
    y
}

