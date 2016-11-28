# \_\_builtin\_expect for Rust


### Description:

We should be able to improve branch prediction in Rust using optional user input for conditionals like `__builtin_expect` in gcc. This LLVM pass uses two strategies to accomplish this. 

1. ~~We adjust which branch immediately follows a conditional (the other requires a `jmp` (or similar) instruction. Assuming the user's calls to `__builtin_expect` are done intelligently (which we are assuming), this should eliminate a branch (and thus an instruction cache miss) in the common case. This assumes that the structure of the LLVM IR we output is actually reflected by the final binary, which could be false if later passes adjust these branches or the layout of the program.~~

2. We adjust branch weight metadata based on the expected truth or falsity of the conditional. This is how `LowerExpectIntrinsic.cpp` handles similar situations with C/C++. This relies on the assumption that LLVM actually uses branch weight metadata and that other passes do not provide their own weight predictions that outweigh ours.

Because it is hard to be certain about the validity of our assumptions, this project should be considered as a proof-of-concept rather than a practical implementation. 

### Design

Moving conditional branches or adjusting their weights requires a handle to all conditionals which rely (directly, for purposes of simplification) on the result of a call to `__builtin_expect`. A FunctionPass is probably not suited for these purposes, so instead an InstVisitor is used with a series of checks done in `visitBranchInst()` to determine whether this branch is one of interest. I believe `LowerExpectIntrinsic.cpp` does a similar check for a branch based on a comparison based on a call to a function of interest. In our case (for now) we identify the function by its name (and we have an empty function of that name in the rust project source); this is sloppy and has the potential for ugly errors (Rust mangles function names, so instead of string equality we are checking whether the function name contains certain text), but it will do for our purposes (modifying the Rust core is outside the scope of this project).

The actual information is communicated using an `MDBuilder` with which we `createBranchWeights()`. 