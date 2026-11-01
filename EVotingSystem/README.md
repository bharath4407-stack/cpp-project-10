# E-Voting System (Local Simulation) — C++17 Console Project

An educational, **fully offline** voting simulation for a college C++ OOP course.
It does **not** connect to any network, election system or government database, and it
uses **only fictional demo data** (please never type real personal information).

## Highlights
* 12 source files, clean separation: **data classes → business logic → console UI**
* OOP: abstract base class, inheritance, polymorphism, encapsulation, constructors, custom exceptions
* STL: `vector`, `map`, `set`, `optional`, `sort`, `copy_if`, `remove_if`, `shuffle`, `<filesystem>`
* Searching (binary + linear) and sorting (`std::sort` with lambdas)
* Persistent text files (`fstream`) with **safe writes**, **corruption recovery** and an **audit log**
* **Secret-ballot design**: `votes.txt` never stores who voted
* 65 automated end-to-end checks (`make test`), verified with AddressSanitizer/UBSan

## Folder structure
```
EVotingSystem/
├── Makefile                 build / run / test / clean
├── CMakeLists.txt           alternative build (Windows/Visual Studio friendly)
├── README.md                this file
├── include/                 HEADER FILES
│   ├── Exceptions.h         custom exception classes
│   ├── Utils.h              validation, hashing, console I/O helpers
│   ├── User.h               abstract base class
│   ├── Voter.h              Voter  : User
│   ├── Admin.h              Admin  : User
│   ├── Candidate.h
│   ├── Vote.h
│   ├── Election.h           state machine NOT STARTED -> OPEN -> CLOSED
│   ├── FileManager.h        all fstream code
│   ├── VotingSystem.h       business rules (the "brain")
│   └── ConsoleUI.h          menus and screens
├── src/                     SOURCE FILES (one .cpp per header + main.cpp)
├── data/                    SAMPLE DATA FILES (demo data, ready to use)
│   ├── voters.txt  candidates.txt  votes.txt
│   ├── admin.txt   election.txt    audit.txt
├── tests/smoke_test.sh      automated end-to-end tests
└── docs/
    ├── PROJECT_REPORT.md    objectives, scope, OOP explanation, tests, viva Q&A ...
    └── sample_output.txt    sample console output
```

## Compile
Requires a C++17 compiler (g++ 8+, clang 7+, MSVC 2019+).

**Linux / macOS / WSL / MinGW (make)**
```bash
cd EVotingSystem
make                # produces ./evoting
```
On MSYS2/MinGW the same file works with `mingw32-make` if plain `make` is not installed.

**Without make (any g++)**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/*.cpp -o evoting
```
On older GCC (8) add `-lstdc++fs` at the end.

**Windows (MinGW g++ in PowerShell/CMD)**
```bat
g++ -std=c++17 -O2 -Iinclude src\*.cpp -o evoting.exe -static-libgcc -static-libstdc++ -static
```
`-static-libgcc -static-libstdc++ -static` make the .exe self-contained (see Troubleshooting).

**Visual Studio / any platform with CMake**
```bash
cmake -S . -B build
cmake --build build --config Release
```

## Run
```bash
./evoting                       # uses the ./data folder (run from the project root)
./evoting --data mydata         # use a different data folder
./evoting --ascii               # '#' instead of the block character in bar charts
./evoting --no-color            # disable colours
make test                       # run the automated tests
```
Colours and screen clearing are enabled automatically only on a real terminal.

## Troubleshooting
**`evoting.exe` starts and exits immediately with code 127, printing nothing (Windows + MinGW/MSYS2)**
A dynamically linked `.exe` must load `libstdc++-6.dll` and `libgcc_s_seh-1.dll` at run time. If an
*incompatible* toolchain's copy is found first on `PATH` — for example you compiled with
`C:\msys64\ucrt64\bin\g++` but an old `C:\msys64\mingw64\bin` is searched first — Windows cannot
load it and the process dies silently. This also makes every `tests/smoke_test.sh` check fail.
Two fixes, either is enough:
```bash
# 1. static runtime (already the default in the Makefile and CMakeLists.txt)
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o evoting.exe -static-libgcc -static-libstdc++ -static
# 2. or put the matching toolchain first for this shell
PATH="/c/msys64/ucrt64/bin:$PATH" bash tests/smoke_test.sh
```
Check the result with `ldd evoting.exe` — no `libstdc++-6.dll` line means it is self-contained.

**`make: command not found` (Windows MSYS2)**
Install it with `pacman -S make`, or use `mingw32-make`, which ships with the toolchain.

If the folder or any file is missing, the program recreates it (with demo data on first run).
To start completely fresh, delete the `data/` folder, or use **Admin → Restore demo data**.

## Demo accounts
| Role  | ID               | Password   |
|-------|------------------|------------|
| Admin | `admin`          | `admin123` |
| Voter | `V1001`–`V1010`  | `pass123`  |

## 2-minute walkthrough
1. Run `./evoting` → **3** (Admin Login) → `admin` / `admin123` → **2** Start election → **19** Logout.
2. **1** (Voter Login) → `V1001` / `pass123` → **3** Cast my vote → choose `101` → `y` → note the receipt.
   Repeat with `V1002`, `V1003`… and different candidates.
3. Admin Login again → **3** Stop election → **12** View final results (table, winner, bar chart).
4. Try the error cases: voting twice, voting before the start, results while OPEN, wrong passwords.
5. Quit and start again: everything is still there (files). Admin → **17** resets the simulation.

See `docs/PROJECT_REPORT.md` for the full project documentation.
