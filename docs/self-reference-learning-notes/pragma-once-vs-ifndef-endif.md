# `#pragma once` vs `#ifndef ... #endif` (include guards)

Both prevent multiple inclusion of headers.

- include guards are preprocessor-based, standard, and more verbose
- `#pragma once` is compiler-level, cleaner, and modern

> [!NOTE]
> - `#pragma` is a compiler directive in C and C++. It tells the compiler to do something special that isn't part of standard C++ syntax.
> - `#pragma something` is like asking the compiler to handle the "something" in a special way.