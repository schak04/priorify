# Make and CMake

> Since I learned Make and CMake for the very first time for this project (although I had been meaning to learn them for quite a while, but it only made sense to do so while actually working on something that required them so the learning actually sticks), I am jotting down some high-level notes about them here for my own revision and this will probably even act as a reference for my future self.

---

## Make

It is a build automation tool. It essentially executes build instructions using a **Makefile**.

Speaking of the problem it solves...

Let's say I have a C++ project with only one source file (`main.cpp`).  
My build instruction will be as simple as:
```bash
g++ main.cpp -o app
```

However, what if:
- there are many (say, 40+) source files,
- many dependencies,
- various libraries, etc.

Doing this thing manually becomes a pain, and THIS is where **Make** comes in: it automates the whole build process.

I'd write:
```makefile
app: main.cpp
    g++ main.cpp -o app
```

Then:
```bash
make
```
It checks what changed and what needs recompilation, and rebuilds efficiently.  
E.g., if only `main.cpp` changes, it only recompiles `main.cpp`.

### Problem with Make

Although it solves one problem, it introduces another.

**What is it?**  
Well, writing Makefiles manually becomes painful for:
- large projects
- cross-platform support
- libraries
- OS differences, etc.

And THAT is where **CMake** comes in.

---

## CMake

It is a **build-system generator**.

We describe our project in a `CMakeLists.txt` file, and CMake generates the build files (like Makefiles, or Ninja files, etc.) automatically for the target platform.

So, the architecture looks something like:

```
CMakeLists.txt -> CMake --(generates Makefiles/Ninja/etc.)--> make/ninja -> compiler executes
```

---
