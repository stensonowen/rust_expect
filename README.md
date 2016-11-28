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

