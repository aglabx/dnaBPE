# DNA BPE Tokenizer

## Prerequisites

- C++ compiler with C++17 support (g++ recommended)
- GNU Make
- Google Test framework for running tests
- pthread support
- Optional: gperftools for profiling

## Installation

1. Clone the repository:
```bash
git clone [repository-url]
cd dnaBPE
```

2. Install Google Test (on Ubuntu/Debian):
```bash
sudo apt-get install libgtest-dev
```

3. For profiling support (optional):
```bash
sudo apt-get install google-perftools libgoogle-perftools-dev
```

## Building

The project includes several build targets:

### Standard Release Build
```bash
make all
```
This creates an optimized executable at `bin/bpe.v5.exe`

### Debug Build
```bash
make debug
```
This creates a debug version at `bin/bpe.debug.exe`

### Profile Build
```bash
make profile
```
This creates a profiling-enabled version at `bin/bpe.profile.exe`

## Running Tests

To compile and run the tests:
```bash
g++ -std=c++17 tests/test_linkedvector.cpp -o bin/tests -lgtest -lgtest_main -pthread
./bin/tests
```

## Profiling

To run the program with profiling:
```bash
make profile-run ARGS="your_input_arguments"
```
This will generate a profile report in `profile_report.txt`

## Build Options

The Makefile includes several optimization flags:
- `-O3`: Highest optimization level
- `-march=native`: CPU-specific optimizations
- `-ffast-math`: Fast math operations
- `-funroll-loops`: Loop unrolling
- `-flto`: Link-time optimization
- `-ftree-vectorize`: Auto-vectorization

## Directory Structure

- `src/`: Source files
- `bin/`: Compiled binaries
- `tests/`: Test files

## Cleaning

To remove all built files:
```bash
make clean
```