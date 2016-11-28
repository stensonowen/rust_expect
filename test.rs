//Test for basic __builtin_expect_ usage

//Compile without stdlib: don't have to link to test
//Pretty sure to compile a *binary* w/o stdlib you need nightly Rust
#![no_std]
#![no_main]
#![feature(lang_items, start)]
#[lang = "eh_personality"] extern fn eh_personality() {}
#[lang = "panic_fmt"] fn panic_fmt() -> ! { loop {} }

#[no_mangle]
// generics apparently require mangling
//	we can mangle a generic function or no_mangle a (bool,bool)
#[inline]
pub fn __builtin_expect_<T: Eq>(expr: T, expect: T) -> bool {
	expr == expect
}

#[no_mangle] 
pub extern fn main(argc: i32, _argv: *const *const u8) -> i32 {
	if __builtin_expect_(argc, 2) {
		1
	} else {
		0
	}
}


