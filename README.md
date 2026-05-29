# Priorify

![C++](https://img.shields.io/badge/C++-blue?style=flat-square&logo=c%2B%2B&logoColor=white)
![SQLite](https://img.shields.io/badge/SQLite-07405E?style=flat-square&logo=sqlite&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-064F8C?style=flat-square&logo=cmake&logoColor=white)
![Bash](https://img.shields.io/badge/Bash-4EAA25?style=flat-square&logo=gnu-bash&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux&logoColor=black)

Priorify is a priority-based task manager built in C++, focused on performance, simplicity, and productivity.  
What started as a basic console app has evolved into a modern, keyboard-driven **Terminal User Interface (TUI)** tailored for terminal-heavy workflows.

---

## Vision

This project is designed to be:
- **Fast:** Minimal overhead with in-memory caching and instant operations.
- **Keyboard-First:** No mouse needed, fully optimised for Vim-style (h/j/k/l) and standard navigation.
- **Focused:** Prioritisation over clutter.
- **Native to Programmers:** Created for programmers who enjoy being in the terminal.

Through this project, I intend to improve the way I track my tasks, solving a personal productivity problem, and I hope it helps others too.

---

## Features (v2.0.0)

- Modern TUI
- **Vim-like Navigation:** `h`/`j`/`k`/`l`
- Custom pill toggles for selecting task status and priority.
- **Task Management:** Add, update, complete, and delete (and clear all) tasks instantly.
- **Visual Feedback:** Colour-coded priority bars and active-state highlights.
- **Persistent Storage:** SQLite database safely stores all task data.
- **Performance Optimised:** In-memory caching minimises database polling.

---

## Technologies Used

- **Language:** C++
- **Database:** SQLite3
- **TUI Framework:** [FTXUI](https://github.com/ArthurSonzogni/ftxui)
- **Build System:** CMake
- **Build Automation:** Bash Scripting

---

## Usage Instructions

### Prerequisites
- CMake
- A C++ compiler supporting C++20 or higher

### On Linux, macOS, and Unix-like Systems

#### 1. Clone the Repository
```bash
git clone https://github.com/schak04/priorify.git
cd priorify
```

#### 2. Build the Project

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

#### 3. Run the App
Always run the executable from the project root to ensure the database path (`data/tasks.db`) resolves correctly.

```bash
./bin/priorify
```

### Keybindings Guide
- `a`: Add a new task
- `e`: Edit selected task
- `d`: Delete selected task (requires confirmation)
- `D`: Clear all tasks (requires confirmation)
- `c`: Toggle completion status
- `j`/`k` or `Up`/`Down`: Navigate lists and form fields
- `h`/`l` or `Left`/`Right`: Cycle through radio options or should I say... pills (Status/Priority)
- `Enter`: Save / Confirm
- `q`/`Esc`: Quit the app when in the dashboard
- `Esc`: Return to the dashboard when inside the **add/edit-task screens** or **delete/clear-all confirmation screens**

> [!NOTE]  
> The app is not available for Windows yet.

---

## System Design

### High-Level Design
- **System Architecture:**  
![System Architecture](docs/diagrams/architecture.png)

- **UML Use Case Diagram:**  
![UML Use Case Diagram](docs/diagrams/use-case.png)

### Low-Level Design
- **Sequence Diagram:**  
![Sequence Diagram](docs/diagrams/sequence-diagram.png)

---

## References
1. [**cppreference**](https://en.cppreference.com/cpp/language)
2. [**SQLite documentation**](https://sqlite.org/docs.html)
3. **Chrono:** [**GfG**](https://www.geeksforgeeks.org/cpp/chrono-in-c) and [**cppreference**](https://en.cppreference.com/cpp/chrono)
4. [**FTXUI documentation**](https://arthursonzogni.github.io/FTXUI)
5. **CMake:** [**documentation**](https://cmake.org/cmake/help/latest/index.html) and [**tutorial**](https://cmake.org/cmake/help/latest/guide/tutorial/index.html#guide:CMake%20Tutorial)
6. [**StackOverflow** - **difference between include guards and #pragma once**](https://stackoverflow.com/questions/22193338/what-is-the-difference-between-ifndef-and-pragma-once-and-what-does-the-same)

---

## Author

&copy; 2025-2026 [Saptaparno Chakraborty](https://github.com/schak04).  
All rights reserved.

---
