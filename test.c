//(OLD) Test for custom __builtin_expect function
//Deprecated in favor of test.cpp; keeping for historical reasons
//NOTE: this was once the standard for testing but now should not be used
//  C doesn't have bools, so it returns integers
//  branches based on integers look different than branches based on bools
//  searching for branches based on a function that returns a bool is cleaner, 
//      easier, and more robust, so we opt to implement only that.
//      That solution works better for C++ and Rust but not at all for C.
//

int __builtin_expect_(int actual, int expected) {
    //actually returns a bool
    return actual == expected;
}

int main() {
    int x = 42;
    if(__builtin_expect_(x > 0, 1)) {
        return 0;
    } else {
        return 1;
    }
}

