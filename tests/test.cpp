//Test for custom __builtin_expect function

bool __builtin_expect_(int actual, int expected) {
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

