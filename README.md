# picayune

A (WIP) C-like programming language compiled using LLVM-IR.

Example code that compiles:
```
function fibonnaci(n: int): int {
    if (n == 0) return 0;
    if (n == 1) return 1;

    return fibonnaci(n - 1) + fibonnaci(n - 2);
}

struct Person {
    name: string,
    age: int
}

function main(): int {
    let person = Person {
        name: "abc",
        age: 21
    };

    return 0;
}
```
