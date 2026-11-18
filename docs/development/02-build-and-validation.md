# Build and validation

The project uses CMake and C++23. One verified environment is WSL Ubuntu with GCC 15.2,
CMake 4.2.3, and Ninja. The root configuration defines the `sandbox` library. `app/` is reserved
for a future executable.

## Configure and build

```sh
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target unit_tests benchmarks_run
```

`BUILD_TESTING` and `BUILD_BENCHMARKS` default to `ON`. They enable GoogleTest and Google
Benchmark respectively. Both dependencies are fetched by CMake from their upstream `main`
branches. For a library-only build, configure with
`-DBUILD_TESTING=OFF -DBUILD_BENCHMARKS=OFF` and build the `sandbox` target.

Sandbox targets treat warnings as errors in every build configuration. GCC and Clang use
`-Wall -Wextra -Wpedantic -Werror`. MSVC-compatible frontends use `/W4 /WX`. The flags are
private to the Sandbox targets, so fetched dependencies retain their own warning policies.

## Compilation databases

CMake generates `compile_commands.json` in the build directory by default with Ninja
or a Makefile generator. Building a Sandbox target also writes its own database at
`compile_commands/<target>/compile_commands.json` within the build directory. The current targets
are `sandbox`, `unit_tests`, and `benchmarks_run`. Set `-DCMAKE_EXPORT_COMPILE_COMMANDS=OFF` to
disable database generation.

To export databases for all enabled Sandbox targets without compiling them, run:

```sh
cmake --build cmake-build-release --target compile_databases
```

## Formatting changed C++ files

The root `.clang-format` was verified with clang-format 18. In a Git checkout, CMake uses
Git, Python 3, and clang-format to format modified or new, non-ignored C++ files beneath
`app/`, `library/`, `tests/`, and `benchmarks/` before compiling a Sandbox target. The
formatter edits whole files in the working tree. Review any changes before staging them.
Use `-DSANDBOX_FORMAT_BEFORE_BUILD=OFF` for a build that leaves source files alone.

Enable the versioned pre-commit hook once per checkout:

```sh
chmod +x .githooks/pre-commit
git config --local core.hooksPath .githooks
```

The hook uses Git, Python 3, and clang-format to check the staged contents of changed C++ files.
It leaves the working tree and the index alone. If a build formats a file you already staged,
review and stage the intended formatting changes before committing. An existing custom hook
setup can call `python3 tools/format_changed.py check-staged` from its own pre-commit hook.
Git for Windows can use the default WSL distribution when native Python 3 is unavailable.

## Run project tests and benchmarks

```sh
ctest --test-dir cmake-build-release/tests --output-on-failure
./cmake-build-release/benchmarks/benchmarks_run --benchmark_min_time=0.01s
```

These commands use the verified WSL environment. Running CTest from the generated `tests/`
directory selects the project tests. The short benchmark run checks that the harness executes.
For comparisons, use equivalent builds and representative inputs and record the environment,
workload, and variability. See the [performance guideline](../../guidelines/08-performance-and-measurement.md).

## Run with assertions enabled

```sh
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_BENCHMARKS=OFF
cmake --build cmake-build-debug --target unit_tests
ctest --test-dir cmake-build-debug/tests --output-on-failure
```

Use Debug to exercise the `VectorView` assertion checks for null pointers and out-of-bounds
access. [View tests](../../tests/math/vectors-view.cpp#L86)

Select validation based on the affected contract. For library templates, instantiate the changed
operations through a consumer and exercise cases that select different implementations. See the
[verification guideline](../../guidelines/07-verification-and-testing.md).
