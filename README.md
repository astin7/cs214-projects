# CS214: Systems Programming

Four projects from Rutgers CS214 (Systems Programming), each tackling a
different core systems concept in C. Together they cover memory management,
POSIX file I/O, process control, and concurrent network programming.

## Projects

### [P1: Custom Memory Allocator](./p1-malloc)
A from-scratch implementation of `malloc()` and `free()`, including usage-error
detection (double-free, invalid pointers), automatic coalescing of adjacent
free chunks, 8-byte alignment, and leak detection registered via `atexit()`.
Includes `memgrind`, a stress-testing harness benchmarking allocation/
deallocation performance across randomized workloads.

**Concepts:** heap management, pointer arithmetic, memory alignment, leak
detection

### [P2: File Similarity Analyzer](./p2-file-similarity)
A program computing the Jensen-Shannon distance between files based on their
word-frequency distributions. Recursively traverses directories, tokenizes
and reads files using raw POSIX I/O (`open()`, `read()`, `close()`), and
ranks all pairwise comparisons by combined word count.

**Concepts:** POSIX I/O, information theory, dynamic data structures,
recursive traversal

### [P3: Simple Shell](./p3-shell)
A POSIX-compliant command-line shell supporting both interactive and batch
modes, I/O redirection, multi-stage pipelines, wildcard globbing, and
built-in commands (`cd`, `pwd`, `which`, `exit`). Uses a single unified
parser and execution loop shared across both modes.

**Concepts:** process spawning, `dup2()`/`pipe()`, command parsing, `execv()`

### [P4: Chat Server](./p4-chat-server)
A multithreaded TCP chat server implementing a custom application-layer
protocol over sockets. Supports concurrent clients, private and broadcast
messaging, user status queries, and structured error handling, using
mutex-protected shared state to safely manage connected users across threads.

**Concepts:** sockets, multithreading, mutual exclusion, application
protocol design

## Tech

C, POSIX APIs (I/O, process control, sockets), pthreads, Make

## Note

All four projects were completed collaboratively with an assigned partner,
per CS214's project structure. Contributions were shared across design,
implementation, and testing for each project.
