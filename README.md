# Operating Systems Coursework — ST5004CEM

This repository contains four tasks covering core operating systems concepts: concurrency and scheduling, memory management, secure file systems, and network/IPC programming. Each task is a standalone C program (or pair of programs) demonstrating a specific set of OS principles.

## Repository Structure

```
Operating-System-CourseWork/
├── Task_1/     Concurrency, race conditions, deadlocks, CPU scheduling
├── Task_2/     Memory management — page replacement algorithms
├── Task_3/     Secure file management system
└── Task_4/     Client-server networking and IPC
```

## Requirements

- Linux or WSL (uses POSIX threads and BSD sockets)
- `gcc`
- `pthread` library (for Task 1 and Task 2's `test.c`)

---

## Task 1 — Concurrency, Race Conditions & Scheduling

Demonstrates thread synchronization problems and their fixes, plus a basic CPU scheduling simulation.

| File | Purpose |
|---|---|
| `test.c` | Minimal pthreads example — creating and joining 3 threads |
| `race.c` | Demonstrates a race condition: 3 threads increment a shared counter with no locking, producing a wrong/non-deterministic result |
| `fixed.c` | Same as `race.c` but protected with a `pthread_mutex_t`, always producing the correct result (300,000) |
| `deadlock.c` | Two threads acquire two locks in *opposite* order, risking deadlock |
| `deadlock_fixed.c` | Same scenario, fixed by having both threads acquire locks in the *same* order |
| `scheduler.c` | Simulates Round-Robin CPU scheduling across 3 processes with a fixed time quantum |

**Build & run:**
```bash
gcc test.c -o test -lpthread && ./test
gcc race.c -o race -lpthread && ./race
gcc fixed.c -o fixed -lpthread && ./fixed
gcc deadlock.c -o deadlock -lpthread && ./deadlock
gcc deadlock_fixed.c -o deadlock_fixed -lpthread && ./deadlock_fixed
gcc scheduler.c -o scheduler && ./scheduler
```

---

## Task 2 — Memory Management (Paging)

Simulates page replacement algorithms over a fixed page reference string `{1,2,3,4,1,2,5,1,2,3,4,5}`.

| File | Purpose |
|---|---|
| `paging.c` | FIFO (First-In-First-Out) page replacement. Takes the number of frames as a command-line argument |
| `paging_lru.c` | LRU (Least Recently Used) page replacement with a fixed 3 frames |
| `test.c` | Basic pthreads warm-up example (same purpose as Task 1's `test.c`) |

Both simulations report page faults, page hits, and hit/fault ratios.

**Build & run:**
```bash
gcc paging.c -o paging && ./paging 3       # 3 = number of frames
gcc paging_lru.c -o paging_lru && ./paging_lru
```

---

## Task 3 — Secure File Management System

A menu-driven, sandboxed file management system (`task3.c`) implementing the security features required by the brief:

- **User authentication** — register/login, passwords stored as FNV-1a hashes (not salted — a deliberate simplification, not production-grade)
- **File permissions** — Unix-style owner/group/other `rwx` permission strings, enforced on read/write; delete is restricted to the owner regardless of the write bit
- **Encryption** — optional XOR-cipher encryption/decryption per file (again a teaching simplification, not cryptographically strong)
- **Audit logging** — every register/login/create/read/write/delete/chmod action is timestamped and logged to `audit.log`
- All managed files live under a sandboxed `./vault/` directory; nothing outside it is touched

**Data files created at runtime:**
- `users.dat` — `username:group:password_hash`
- `filesystem.meta` — `filename:owner:group:permissions:encrypted`
- `audit.log` — action history

**Build & run:**
```bash
gcc task3.c -o task3 && ./task3
```
Menu options: Register → Login → Create/Read/Write/Delete file, change permissions, list files, view audit log.

---

## Task 4 — Client-Server IPC (TCP Sockets)

A TCP client-server pair demonstrating network programming, process-per-connection concurrency, and basic protocol security.

**`server.c`**
- Listens on port `5050`
- Forks a child process per incoming connection (`fork()` + `SIGCHLD` reaping to avoid zombies)
- Requires username/password authentication before accepting commands (hardcoded demo credentials: `alice/pass123`, `bob/hunter2`)
- Validates all input (rejects control characters / oversized lines) before processing
- Supports commands: `ECHO <text>`, `TIME`, `UPPER <text>`, `QUIT`
- Logs every connection, auth attempt, and command to stdout with timestamps

**`client.c`**
- Connects to the server (defaults to `127.0.0.1`, or pass an IP as an argument)
- Prompts for username/password, then lets you type commands interactively

**Build & run:**
```bash
# Terminal 1
gcc server.c -o server && ./server

# Terminal 2
gcc client.c -o client && ./client
# or: ./client <server_ip>
```

