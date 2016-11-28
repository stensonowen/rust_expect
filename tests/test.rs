//Test for basic __builtin_expect_ usage

//Compile without stdlib: don't have to link to test
//Pretty sure to compile a *binary* w/o stdlib you must use nightly Rust
#![no_std]
#![no_main]
#![feature(lang_items, start)]
#[lang = "eh_personality"] extern fn eh_personality() {}
#[lang = "panic_fmt"] fn panic_fmt() -> ! { loop {} }

const ITERS: f64 = 10_000_000_000.;

#[no_mangle]
#[inline]
pub fn __builtin_expect_(expr: bool, expect: bool) -> bool {
	expr == expect
}

#[no_mangle]
#[inline]
pub fn __blank_function_(expr: bool, expect: bool) -> bool {
	expr == expect
}

#[no_mangle] 
pub extern fn main(argc: i32, _argv: *const *const u8) -> i32 {
    let fn_to_test = _builtin_test;
    //let fn_to_test = _control;

    fn_to_test(argc as f64) as i32
}

fn _control(mut y: f64) -> f64 {
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

fn _builtin_test(mut y: f64) -> f64 {
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

