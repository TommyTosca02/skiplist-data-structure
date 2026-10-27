# Skip List Project - Advanced Algorithms Individual Challenge

* **Student**: Tommaso Tosca
* **Student ID**: 10804131
* **Course**: Advanced Algorithms and Parallel Programming (095946)
* **Academic Year**: 2026-2027
* **Language**: C17 & C++17 

---

## Description

This project is my implementation of a randomized Skip List in C17 for signed 64-bit integer keys (`int64_t`).

It implements four main operations:
1. `skiplist_search`: Searches for a key in O(log N) expected time.
2. `skiplist_insert`: Inserts a key if absent, rejecting duplicates.
3. `skiplist_split`: Structurally splits a list around a pivot into two lists (`left_split` with keys <= pivot, `right_split` with keys > pivot) by rewiring existing pointers without allocating new data nodes.
4. `skiplist_union`: Structurally merges two ordered disjoint lists (`max(skiplist1) < min(skiplist2)`) by rewiring pointers.

It also includes an invariant checker (`skiplist_check_invariants`) and a rebuilding version of split (`skiplist_split_rebuilding`) used as a performance baseline.

---

## File Structure

* `skiplist.h`: Header file with struct definitions and function prototypes.
* `skiplist.c`: C17 implementation of the Skip List and core functions.
* `test_main.c`: Test suite checking edge cases, 64-bit limits, address preservation, roundtrip property, and 1,000 mixed operations on 3 seeds.
* `benchmark_driver.cpp`: C++ driver using Google Benchmark to measure performance  across 6 problem sizes (N = 1024 to 1048576).
* `plot_results.py`: Python script reading `raw_results.json` to create `scaling_plot.png`.
* `report.pdf`: Two-page A4 summary report.

---

## Environment and Tools

* **Compiler**: MSVC (Visual Studio 2022 / cl compiler) or GCC 11+ / Clang 12+.
* **Standards**: C17 for C source files, C++17 for the benchmark driver.
* **Libraries**: Google Benchmark v1.10.0 and Python `matplotlib`.

---

## Build Everything Using CMake 

**On Windows**:
```powershell
if (!(Test-Path benchmark)) { git clone https://github.com/google/benchmark.git benchmark }
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -E make_directory benchmark/build
& $cmake -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -S benchmark -B benchmark/build
& $cmake --build benchmark/build --config Release
& $cmake -B build
& $cmake --build build --config Release
.\build\Release\test_main.exe
.\build\Release\benchmark_runner.exe --benchmark_out=results/raw_results.json --benchmark_out_format=json
python scripts/plot_results.py
```

**On Linux / Mac**:
```bash
if [ ! -d benchmark ]; then git clone https://github.com/google/benchmark.git benchmark; fi
cmake -B benchmark/build -S benchmark -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
cmake --build benchmark/build --config Release
cmake -B build
cmake --build build --config Release
./build/test_main
./build/benchmark_runner --benchmark_out=results/raw_results.json --benchmark_out_format=json
python3 scripts/plot_results.py
```

## Alternative: Direct Compilation (Without CMake)

### 1. Correctness Tests (C17)

**On Windows**:
```powershell
New-Item -ItemType Directory -Force build, results | Out-Null
cmd /c 'call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cl /nologo /std:c17 /Fo:build\ src\skiplist.c tests\test_main.c /Febuild\test_main.exe'
if ($LASTEXITCODE -eq 0) { .\build\test_main.exe }
```

**On Linux / Mac**:
```bash
mkdir -p build results
gcc -std=c17 -Isrc src/skiplist.c tests/test_main.c -o build/test_main
./build/test_main
```

---

### 2. Compile Google Benchmark Library

**On Windows**:
```powershell
if (!(Test-Path benchmark)) { git clone https://github.com/google/benchmark.git benchmark }
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -E make_directory benchmark/build
& $cmake -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -S benchmark -B benchmark/build
& $cmake --build benchmark/build --config Release
```

**On Linux / Mac**:
```bash
if [ ! -d benchmark ]; then git clone https://github.com/google/benchmark.git benchmark; fi
mkdir -p benchmark/build
cmake -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -S benchmark -B benchmark/build
cmake --build benchmark/build --config Release
```

---

### 3. Run Benchmarks and Export JSON

**On Windows**:
```powershell
cmd /c 'call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cl /nologo /std:c++17 /EHsc /O2 /MD /DBENCHMARK_STATIC_DEFINE /Fo:build\ /Isrc /Ibenchmark\include src\skiplist.c src\benchmark_driver.cpp /Febuild\benchmark_runner.exe /link benchmark\build\src\Release\benchmark.lib benchmark\build\src\Release\benchmark_main.lib Shlwapi.lib'
if ($LASTEXITCODE -eq 0) { .\build\benchmark_runner.exe --benchmark_out=results/raw_results.json --benchmark_out_format=json }
```

**On Linux / Mac**:
```bash
mkdir -p build results && \
gcc -O2 -Isrc -c src/skiplist.c -o build/skiplist.o && \
g++ -std=c++17 -O2 -pthread \
    -DBENCHMARK_STATIC_DEFINE \
    -Isrc -Ibenchmark/include \
    build/skiplist.o src/benchmark_driver.cpp \
    benchmark/build/src/libbenchmark_main.a \
    benchmark/build/src/libbenchmark.a \
    -o build/benchmark_runner && \
./build/benchmark_runner \
    --benchmark_out=results/raw_results.json \
    --benchmark_out_format=json
```

---

### 4. Generate Performance Plot

**On Windows**:
```powershell
python scripts/plot_results.py
```

**On Linux / Mac**:
```bash
python3 scripts/plot_results.py
```

---

## Documented Seeds

Node heights are generated using a xorshift64. The seeds used across tests and benchmarks are:
* **Seed 42** 
* **Seed 234**
* **Seed 1000**

---

## Memory & Correctness Verification

Verified using AddressSanitizer:
* No memory errors

Verified using test_main.c:
* Consumed input lists remain valid, empty (`head->next[i] == NULL`), and reusable.

---

## Known Limitations

1. **Fixed Max Height**: height limited at `SKIPLIST_MAX_HEIGHT = 32'.
2. **Single-threaded**: designed for a single thread.

---

## Declaration of Work and AI Assistance

This project was completed individually.

* **References**: Course lecture on Randomized Data Structures; W. Pugh, *A Skip List Cookbook* (1990); Google Benchmark documentation.
* **AI Assistance**: Gemini was used as an interactive tutor for concept clarification, C17 and C++17 syntax discussions, correcting my english here and in the report, error detections and corrections, writing `plot_results.py`, writing the code of skiplist.c where we distinguish between _BitScanForward64() and __builtin_ctzll() depending on whether we are on Windows or Linux/Mac, writing xorshift64(), writing CMakeLists.txt and terminal debugging assistance during development. All final code, tests, and analysis were done by me.