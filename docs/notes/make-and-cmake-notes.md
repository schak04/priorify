# Make and CMake

> Since I learned Make and CMake for the very first time for this project (although I had been meaning to learn them for quite a while, but it only made sense to do so while actually working on something that required them so the learning actually sticks), I am jotting down some high-level notes about them here for my own revision and this will probably even act as a reference for my future self.

---

## Make

It is a **build automation tool**. It automates the build process using rules defined in a **Makefile**.

Speaking of the problem it solves...
Let's say I have a small C++ project:

```text
project/
├── main.cpp
├── math.cpp
├── math.h
```

Without Make, I might compile it manually like this every time:

```bash
g++ main.cpp math.cpp -o app
```

This works, but as projects grow larger, recompiling everything manually becomes tedious and inefficient.

With Make, I can define dependencies in a `Makefile`:

```makefile
app: main.o math.o
	g++ main.o math.o -o app

main.o: main.cpp math.h
	g++ -c main.cpp

math.o: math.cpp math.h
	g++ -c math.cpp
```

> [!NOTE]  
> A .o file is a compiled object file that contains unlinked machine code meant for a linker. An extensionless binary file is typically a finalised, fully-linked executable (or another custom binary format) that the operating system can run directly as a program.  
> `g++ main.cpp math.cpp -o app` is a full-build command that requires recompilation of unchanged files along with changed/updated source code files. On the other hand, `g++ main.o math.o -o app` is a link-only command that just links machine code files together to form a final executable, and for that, we make these `.o`-extensioned object binary files using `g++ -c ...` so that we only recompile the source code files that have been changed.

Now I can simply run:

```bash
make
```

It checks what changed and what needs recompilation, and rebuilds efficiently.

For example:

- if only `math.cpp` changes, only `math.o` is rebuilt,
- then the executable is relinked,
- `main.cpp` is not recompiled unnecessarily.

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
