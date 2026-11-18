# Document I/O benchmarks

[The benchmark](../document.cpp) builds the same configuration-shaped document for every case.
Each record has an ID, an enable flag, a name, nested limits, and an array of string tags.
Setup prepares the in-memory inputs and checks round trips before timing starts. Each iteration
includes the operation, its allocations, and destruction of its result. It excludes filesystem I/O.
JSON uses compact output, and YAML uses block output. Conversion to core starts with a
`DocumentValue` that has numeric type hints.

## Run

Use the Release configuration from [Build and validation](../../../docs/development/02-build-and-validation.md).

```sh
cmake --build cmake-build-release --target benchmarks_run
./cmake-build-release/benchmarks/benchmarks_run \
  --benchmark_filter=bm_document_ --benchmark_min_time=0.08s --benchmark_repetitions=7 \
  --benchmark_out=cmake-build-release/document-benchmarks.json --benchmark_out_format=json
```

The codec benchmarks report bytes processed from each format's own document size. All six
operations also report records processed per second.

## Initial measurements

Measured on 19 November 2026 at `87025fe` with the document I/O changes in the working tree.
WSL2 Ubuntu ran on an Intel Core i7-10870H with GCC 15.2, CMake 4.2.3, Google Benchmark 1.10.0,
and LibYAML 0.2.5. The build used C++23, `-O3`, and `-DNDEBUG`.

Median CPU time per operation in microseconds across seven repetitions.

| Operation | 16 records | 256 records | 4,096 records | 4,096 range |
| --- | ---: | ---: | ---: | ---: |
| Parse JSON | 22.6 | 362 | 7,313 | 6,898–9,691 |
| Write JSON | 5.58 | 84.7 | 1,957 | 1,828–2,415 |
| Parse YAML | 74.7 | 1,189 | 22,173 | 20,369–25,587 |
| Write YAML | 38.5 | 591 | 10,580 | 9,969–13,309 |
| Core → document | 11.3 | 200 | 5,120 | 4,573–6,273 |
| Document → core | 12.0 | 251 | 5,555 | 5,081–5,828 |

The 4,096-record document is 543,728 bytes as JSON and 654,319 bytes as YAML. For this workload,
YAML parsing took about 3.0 times the CPU time of JSON parsing, and YAML writing took about
5.4 times the CPU time of JSON writing. These results describe the current codecs and workload.
The text sizes differ, and the harness exercises repeated operations on the same inputs.
