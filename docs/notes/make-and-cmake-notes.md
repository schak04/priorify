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

### Makefile syntax in full

The core structure of any rule in a Makefile is:

```makefile
target: dependencies
	recipe
```

- **target** - the file to produce, or the name of an action (called a phony target).
- **dependencies** - files the target depends on. If any of these are newer than the target, the recipe runs.
- **recipe** - shell commands to run. Each line must be indented with a **tab**, not spaces.

#### Variables

```makefile
CC = g++
CFLAGS = -Wall -std=c++17
TARGET = app
SRCS = main.cpp math.cpp
OBJS = $(SRCS:.cpp=.o)  # replaces .cpp with .o for each item in SRCS

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
```

The `%` is a wildcard that matches any stem, so `%.o: %.cpp` means "for any `.o` file, depend on the corresponding `.cpp` file".

The automatic variables are important here:

| Variable | Meaning |
|---|---|
| `$@` | the target name |
| `$<` | the first dependency |
| `$^` | all dependencies |
| `$*` | the stem matched by `%` |

#### Phony targets

A phony target is not a real file, it is just a label for a set of commands. Without declaring it as phony, Make would look for a file with that name and might get confused.

```makefile
.PHONY: clean all

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
```

Running `make clean` would then delete all object files and the binary.

#### A more complete, practical Makefile

Here is what a typical project Makefile looks like when it is all put together (C++ project example):

```makefile
CC = g++
CFLAGS = -Wall -Wextra -std=c++17
LDFLAGS =
TARGET = app

SRC_DIR = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
```

A few things worth noting here:

- `$(wildcard ...)` - expands glob patterns, giving all .cpp files in the src directory.
- `$(patsubst pattern, replacement, text)` - substitutes pattern with replacement for each word in text.
- `| $(BUILD_DIR)` is an **order-only prerequisite**, it ensures the build directory exists before trying to write `.o` files into it, but if the directory already exists and its timestamp changes, it does not trigger a rebuild of all object files.

#### Including headers in dependency tracking

One issue with the basic setup is that Make does not automatically know which `.cpp` files include which headers. If I change `math.h`, Make might not recompile `main.cpp` even though it should.

The fix is to generate dependency files:

```makefile
DEPS = $(OBJS:.o=.d)

-include $(DEPS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
```

- `-MMD` tells the compiler to generate a `.d` file alongside the `.o` file, listing all headers the source depends on.
- `-MP` adds empty dummy rules for each header so Make does not error if a header gets deleted.
- `-include` (with the leading dash) means "include these files if they exist, silently skip if they don't".

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

### The basic workflow

```bash
mkdir build && cd build
cmake ..
make
```

Or with Ninja instead of Make:

```bash
cmake -G Ninja ..
ninja
```

The `..` points CMake to the directory containing `CMakeLists.txt`. All generated files land inside `build/`, keeping the source tree clean. This is called an **out-of-source build** and it is the way to go.

### CMakeLists.txt syntax

Every `CMakeLists.txt` starts with two mandatory lines:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyApp)
```

- `cmake_minimum_required` guards against old CMake versions that might not support certain features used in the project.
- `project` names the project and optionally declares the languages used.

```cmake
project(MyApp VERSION 1.0 LANGUAGES CXX)
```

#### Adding an executable

```cmake
add_executable(app main.cpp math.cpp)
```

This creates a target called `app` from those source files. CMake handles all the object file compilation and linking internally.

#### Adding a library

```cmake
add_library(mathlib math.cpp)
```

By default this creates a **static library** (`libmathlib.a`). For a shared library:

```cmake
add_library(mathlib SHARED math.cpp)
```

Then link it to the executable:

```cmake
target_link_libraries(app PRIVATE mathlib)
```

The keywords `PRIVATE`, `PUBLIC`, and `INTERFACE` control how dependencies propagate:

| Keyword | Meaning |
|---|---|
| `PRIVATE` | only `app` links against `mathlib`; nothing that links `app` gets `mathlib` automatically |
| `PUBLIC` | `app` links against `mathlib`, and anything linking `app` also links `mathlib` |
| `INTERFACE` | `app` itself does not use `mathlib` directly, but anything linking `app` does |

For most executables, `PRIVATE` is what we want.

#### Include directories

```cmake
target_include_directories(app PRIVATE include/)
```

This tells the compiler where to look for header files for that target. Again `PRIVATE/PUBLIC/INTERFACE` applies the same way.

#### Compile options and definitions

```cmake
target_compile_options(app PRIVATE -Wall -Wextra)
target_compile_definitions(app PRIVATE DEBUG_MODE)
```

The definition above is equivalent to passing `-DDEBUG_MODE` to the compiler.

#### Setting the C++ standard

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Setting `CMAKE_CXX_EXTENSIONS OFF` disables compiler-specific extensions (like GCC's `-std=gnu++17`) and forces the strict standard (`-std=c++17`). Good for portability.

Or on a per-target basis:

```cmake
set_target_properties(app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
```

### Variables and control flow

CMake has its own scripting language.

```cmake
set(MY_VAR "hello")
message(STATUS "The value is: ${MY_VAR}")
```

`message(STATUS ...)` prints during the configure step. Other levels are `WARNING`, `SEND_ERROR`, `FATAL_ERROR`.

#### Lists

```cmake
set(SOURCES main.cpp math.cpp utils.cpp)
# or equivalently
set(SOURCES
    main.cpp
    math.cpp
    utils.cpp
)
add_executable(app ${SOURCES})
```

Lists in CMake are semicolon-separated under the hood. When we write them with spaces or newlines in a `set()` call, CMake joins them with semicolons internally.

#### Conditionals

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(app PRIVATE DEBUG_MODE)
endif()
```

The operators I find myself using most: `STREQUAL`, `EQUAL`, `LESS`, `GREATER`, `AND`, `OR`, `NOT`, `DEFINED`.

#### Loops

```cmake
foreach(src IN LISTS SOURCES)
    message(STATUS "Source file: ${src}")
endforeach()
```

### Build types

CMake has a few built-in build configurations we can pass at configure time:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

- **Debug** - no optimisations, debug symbols (`-g`)
- **Release** - optimised (`-O3`), no debug info
- **RelWithDebInfo** - optimised but with debug symbols

We can check this inside CMakeLists.txt:

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build")
endif()
```

### Finding and using external libraries

CMake can find installed system libraries using `find_package`:

```cmake
find_package(OpenSSL REQUIRED)

target_link_libraries(app PRIVATE OpenSSL::SSL OpenSSL::Crypto)
```

- `REQUIRED` makes CMake error out if the library is not found.
- The `OpenSSL::SSL` style names are **imported targets**, they carry all the include paths, link flags, and definitions automatically, so no separate `target_include_directories` call is needed.

For libraries that do not have a built-in CMake find module, we fall back to `pkg-config`:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

target_include_directories(app PRIVATE ${GTKMM_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${GTKMM_LIBRARIES})
target_compile_options(app PRIVATE ${GTKMM_CFLAGS_OTHER})
```

### Subdirectories and multiple CMakeLists.txt files

For larger projects, we split things across directories. Each directory can have its own `CMakeLists.txt`:

```text
project/
├── CMakeLists.txt          <- root
├── src/
│   ├── CMakeLists.txt
│   └── main.cpp
└── libs/
    ├── math/
    │   ├── CMakeLists.txt
    │   ├── math.cpp
    │   └── math.h
    └── utils/
        ├── CMakeLists.txt
        ├── utils.cpp
        └── utils.h
```

Root `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs/math)
add_subdirectory(libs/utils)
add_subdirectory(src)
```

`libs/math/CMakeLists.txt`:

```cmake
add_library(math math.cpp)
target_include_directories(math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

Using `PUBLIC` here means that any target linking `math` automatically gets the include path for its headers too - no need to repeat `target_include_directories` in every consumer.

`src/CMakeLists.txt`:

```cmake
add_executable(app main.cpp)
target_link_libraries(app PRIVATE math utils)
```

`${CMAKE_CURRENT_SOURCE_DIR}` is the directory of whichever `CMakeLists.txt` is currently being processed. Some other built-in variables worth knowing:

| Variable | Meaning |
|---|---|
| `CMAKE_SOURCE_DIR` | root source directory (where the top-level CMakeLists.txt is) |
| `CMAKE_BINARY_DIR` | root build directory |
| `CMAKE_CURRENT_SOURCE_DIR` | directory of the current CMakeLists.txt being processed |
| `CMAKE_CURRENT_BINARY_DIR` | build directory corresponding to the current source directory |
| `PROJECT_NAME` | the name given to the most recent `project()` call |
| `CMAKE_BUILD_TYPE` | Debug, Release, etc. |
| `CMAKE_CXX_COMPILER` | the C++ compiler being used |

### Custom options

We can expose build options to the user with `option()`:

```cmake
option(ENABLE_TESTS "Build the test suite" OFF)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
```

Which we can then toggle when configuring:

```bash
cmake -DENABLE_TESTS=ON ..
```

### Installing targets

When we want `make install` to work:

```cmake
install(TARGETS app DESTINATION bin)
install(TARGETS mathlib DESTINATION lib)
install(FILES include/math.h DESTINATION include)
```

The install prefix defaults to `/usr/local` on Linux. To change it:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/myapp ..
```

Then run `make install` (or `sudo make install` if installing system-wide).

### Generator expressions

Generator expressions are the `$<...>` syntax. Unlike regular variables, they get evaluated at build time rather than configure time, so they can react to things like the current build config:

```cmake
target_compile_options(app PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

target_compile_definitions(app PRIVATE
    $<$<BOOL:${ENABLE_WARNINGS}>:WARN_ENABLED>
)
```

- `$<CONFIG:Debug>` - evaluates to 1 if the current build config is Debug, 0 otherwise.
- `$<$<BOOL:${VAR}>:value>` - includes `value` only if `VAR` is truthy.

They look a bit weird at first but save us from scattering if/endif blocks everywhere.

### A complete, realistic CMakeLists.txt

Putting it all together, here is what the root-level file for a real project looks like:

```cmake
cmake_minimum_required(VERSION 3.20)
project(Priorify VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_WARNINGS "Enable extra compiler warnings" ON)
option(BUILD_TESTS "Build unit tests" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

`src/CMakeLists.txt`:

```cmake
set(SOURCES
    main.cpp
    window.cpp
    task_manager.cpp
)

add_executable(priorify ${SOURCES})

target_include_directories(priorify PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GTKMM_INCLUDE_DIRS}
)

target_link_libraries(priorify PRIVATE
    ${GTKMM_LIBRARIES}
)

target_compile_options(priorify PRIVATE
    ${GTKMM_CFLAGS_OTHER}
    $<$<BOOL:${ENABLE_WARNINGS}>:-Wall -Wextra -Wpedantic>
)

target_compile_definitions(priorify PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
    APP_VERSION="${PROJECT_VERSION}"
)

install(TARGETS priorify DESTINATION bin)
```

### Useful CMake commands cheatsheet

```bash
cmake -S . -B build                     # configure, source in . build in build/
cmake -S . -B build -G Ninja            # use Ninja instead of Make
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build                     # build the project
cmake --build build --target clean      # run the clean target
cmake --build build -j$(nproc)          # parallel build
cmake --install build                   # install
cmake --build build --config Release    # for multi-config generators (MSVC, Xcode)
```

The `-S` and `-B` flags are the modern way to configure - no need to `cd` into the build directory manually.
