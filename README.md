proto-eq
================

Mathematical equations to C++ using expression templates and constraint solving (prototype)

Example:

    eq::Var<int> x, y;
    rel(x*2 == y && y + 1 == x - 1);

    std::cout << x << ", " << y << std::endl; // -2, -4

Another example:

	eq::PriorityVar low, med, high;
	rel(low < med && med < high);

	eq::Var<int> x;
	rel(x == 1, high);
	rel(x == 2, med);
	rel(x == 3, low);

	std::cout << x << std::endl; // 1

Missing features:

- copy for `eq::Var<T>`
- removing single relations
- support for floating point values
- support for user-defined types
- support for user-defined operators/functions
- tests
