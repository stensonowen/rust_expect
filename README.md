# \_\_builtin\_expect for Rust


### Description:

We should be able to improve branch prediction in Rust using optional user input for conditionals like `__builtin_expect` in gcc. This LLVM pass uses two strategies to accomplish this. 

1. ~~We adjust which branch immediately follows a conditional (the other requires a `jmp` (or similar) instruction. Assuming the user's calls to `__builtin_expect` are done intelligently (which we are assuming), this should eliminate a branch (and thus an instruction cache miss) in the common case. This assumes that the structure of the LLVM IR we output is actually reflected by the final binary, which could be false if later passes adjust these branches or the layout of the program.~~

2. We adjust branch weight metadata based on the expected truth or falsity of the conditional. This is how `LowerExpectIntrinsic.cpp` handles similar situations with C/C++. This relies on the assumption that LLVM actually uses branch weight metadata and that other passes do not provide their own weight predictions that outweigh ours.

Because it is hard to be certain about the validity of our assumptions, this project should be considered as a proof-of-concept rather than a practical implementation. 

### Design

Moving conditional branches or adjusting their weights requires a handle to all conditionals which rely (directly, for purposes of simplification) on the result of a call to `__builtin_expect`. 
A FunctionPass is probably not suited for these purposes, so instead an InstVisitor is used with a series of checks done in `visitBranchInst()` to determine whether this branch is one of interest. 
I believe `LowerExpectIntrinsic.cpp` does a similar check for a branch based on a comparison based on a call to a function of interest. 
(The InstVisitor is performed one module at a time using a ModulePass.)
In our case (for now) we identify the function by its name (and we have an empty function of that name in the rust project source); this is sloppy and has the potential for ugly errors (Rust mangles function names\*, so instead of string equality we are checking whether the function name contains certain text), but it will do for our purposes (modifying the Rust core is outside the scope of this project).

The actual information is communicated using an `MDBuilder` with which we `createBranchWeights()`. 

\*: We can use `#[no_mangle]` to avoid function name mangling, but it throws a warning/error for a generic function like the one I'm using. The compiler generates multiple versions with names for each type the function is used with, so the issue depends on our use of `__builtin_expect_`: either we only ever use it with one type and avoid mangling (and suck up the warning) or we can use it with any number of types in the program and permit mangling to prevent a compiler error.

### Recognizing Relevant Calls

Determining which branches we were interested turned out to be slightly more interesting than expected. While first writing and testing the pass I used a basic C function to recognize by name:
```C
    int __builtin_expect_(int actual, int expect) {
        return actual == expect;
    }
```

This produced LLVM IR with the following pattern:
```LLVM
    %5 = call i32 @__builtin_expect_(i32 %4, i32 1)
    %6 = icmp ne i32 %5, 0
    br i1 %6, label %7, label %8
```
This is the pattern I originally matched, and it is the pattern which `LowerExpectIntrinsic.cpp` matches. However, this was problematic when applying it to Rust, because my Rust version returns a boolean instead of an integer that should be either 0 or 1. So the IR changed.
```LLVM
start:                                            ; preds = %entry-block
    %2 = call zeroext i1 @_ZN4test17__builtin_expect_17h8530ac139335dd58E(i32 %0, i32 2)
    br label %bb1

bb1:                                              ; preds = %start
    br i1 %2, label %bb2, label %bb3
```
To confirm this, my pass no longer works when run on a C++ version of the original C file but with a return value of `bool` instead of `int` (which is understandable from the IR).
```LLVM
    %5 = call zeroext i1 @_Z17__builtin_expect_ii(i32 %4, i32 1)
    br i1 %5, label %6, label %7

```
Fortunately the changes are pretty straightforward; we just eliminate a comparison (which previously allowed us to branch based on an integer return value) from our pattern check and end up with something easier to follow and harder to fool. However, this means we aren't quite as language-agnostic as we might have been. This is not a big deal and is certainly not a priority; it might be interesting to replace pattern checking itself with traversing a def-use chain to see if a branch conditional depends in any way on a `__builtin_expect_` call. 
That would be convenient because it would make the solution independent of language or unexpected optimizations or major compiler updates, but it seems like that could quickly get very complex and is outside the scope of this project.

### Benchmarks

Our proof of concept proved the concept! Ceteris paribus, our branch weight information speeds up a simplistic test by a little under 2%. 

In order to isolate the effect of the branch likelihood metadata, I tested the speed of `__builtin_expect_` against an identical function with a different name (so it wouldn't get optimized by the pass).
The function tested is a pretty trivial example: we perform some arithmetic inside a loop with a fairly predictable conditional.
```Rust
fn builtin_test(mut y: f64) -> f64 {
    let mut count: f64 = 0.0;
    while count < ITERS {
        count += 1.0;
        if __builtin_expect_(count > 5.0,true) {
        //the control calls an identical function 
        // with only a different name
            y += 1.0;
        } else {
            y += 2.0;
        }
    }
    y
}
```

It would be more practical but less interesting to test against a control function without an added function because the performance comparison would depend on our example. This just serves as a demo that there is some possible speedup.

I'm not using Rust's handy benchmarking features because they require linking with the standard library; I'm instead using GNU Time's `user` time field. The test's counter is also based on floats because integer types in Rust can `panic` if they over/underflow and `panic`ing requires the standard library. I think you can avoid such runtime checks by compiling in release mode, but that adds in optimizations that might not play nicely with our proof-of-concept pass.

After five randomly interleaved iterations of each test (10,000,000,000 iterations, ~30 seconds), we see a small but clear gap. 

| Test:     |   1   |   2   |   3   |   4   |   5   |   Avg |
| --------- |------:|------:|------:|------:|------:|------:|
| Control   |28.884 |29.032 |29.024 |28.948 |28.784 |28.934 |
| Builtin   |28.652 |28.276 |28.504 |28.280 |28.568 |28.456 |

It isn't huge, but there's definitely a difference; the longest time with our pass is 0.5% faster than the slowest time without. 


### Future Work

There is plenty that can be done to transform this demo from a proof-of-concept into something that is actually useful. 

* Incorporating `__builtin_expect` into the Rust core instead of just searching for a function by name

* Adding custom branch weights for `match` statements

* Adding generic comparisons rather than just booleans
