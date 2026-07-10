# BASH_SOURCE vs $0

In Bash scripting, `${BASH_SOURCE[0]}` is an internal environment variable that contains the path or filename of the currently executing script or source file. 

> `$BASH_SOURCE` and `${BASH_SOURCE[0]}` are functionally identical when referencing the current file. Because BASH_SOURCE is an internal array, referencing it without an index automatically defaults to the first element (index 0).

- `${BASH_SOURCE[0]}` returns the filename where the code physically lives.
- `$0` returns the name of the process calling the code.

`${BASH_SOURCE[0]}` is always the current file. Elements like `${BASH_SOURCE[1]}` show the file that called it, forming a backtrace.
