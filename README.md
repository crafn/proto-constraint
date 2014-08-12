proto-eq
================

Mathematical equations to C++ using expression templates and constraint solving (prototype)

Example:

    eq::Var<int> x, y;
    rel(x + x == y && y + 1 == x - 1);
    std::cout << x << ", " << y; // -2, -4

Missing features:

- copy & move for `eq::Var<T>`
- support for floating point values
- support for user-defined types
- support for user-defined operators/functions
