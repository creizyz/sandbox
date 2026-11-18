---
name: benchmark-change
description: Measure a Sandbox performance question or compare an implementation with a baseline under controlled conditions. Use when timing, scaling, allocation, or other quantitative evidence is the requested outcome.
---

# Benchmark a change

1. Define the operation, metric, representative inputs, and comparison. Identify setup included
   in the measurement. Use the [project build guide](../../../docs/development/02-build-and-validation.md)
   for the harness and the [performance guideline](../../../guidelines/08-performance-and-measurement.md)
   for evidence quality.
2. Verify correctness and compare equivalent compiler settings, workloads, and measurement
   boundaries. For microbenchmarks, check that the measured work survives optimization and that
   input setup and barriers are accounted for. Disable formatting if it would alter a baseline.
3. Repeat runs to assess variability. Report absolute results, relevant relative differences,
   build and workload details, and limits of the comparison. Treat a smoke run as evidence that
   the harness executes. Save measurements or modify the implementation within the requested scope.
