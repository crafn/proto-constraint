proto-constraint
================

Constraints to C++ (prototype)

Goal:

    constraint::Value<int> x, y;
    rel(x*2 == y && y + 1 == x - 1);
    std::cout << x << ", " << y; // -2, -4

Current state:

    constraint::Domain d;
    constraint::Value<int> x{d}, y{d};
    rel(x + x == y && y + 1 == x - 1);
    std::cout << x << ", " << y; // -2, -4
