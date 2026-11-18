# Performance and measurement

Use these standards when comparing resource costs, setting budgets or justifying optimization.
Project documentation supplies actual workloads, commands, hardware requirements and thresholds.

## Define the question

Name the metric and the user or system consequence: latency, throughput, memory, allocations,
startup cost, bandwidth or another constrained resource. Specify units, the measured boundary and
the acceptance criterion. Distinguish elapsed time, CPU time, GPU time and end-to-end latency.
Improving a proxy is useful only when its relationship to the requirement is understood.

Choose representative workloads, sizes and operating conditions. Include relevant worst cases or
growth regimes without presenting a synthetic stress case as typical usage. State assumptions
behind asymptotic analysis; complexity bounds do not establish actual constants, locality or
contention costs.

## Comparable experiments

Retain a baseline and compare under equivalent inputs, configuration and measurement boundaries.
Record build mode, revision, compiler/tool versions, hardware, drivers and relevant runtime options.
Separate setup and warmup from steady state when the question requires it; measure cold behavior
separately when it matters. Verify that the optimized implementation still performs the required
work and produces correct results.

Repeat measurements enough to assess variability. Choose statistics appropriate to the claim:
an average alone does not establish tail latency, and a ratio from one short run does not establish
a reliable speedup. Report sample size, dispersion and material noise sources. Use paired workloads
where appropriate and avoid claiming significance below the experiment's resolution.

Account for measurement overhead, synchronization, caching, background activity and instrumentation.
In asynchronous systems, ensure the chosen boundary includes the completion being claimed.
Do not compare a validation-instrumented run with an uninstrumented run without explaining the
difference and its effect.

## Decisions and durable results

Optimize a demonstrated bottleneck or an explicit budget violation. Compare the gain with code
complexity, memory, accuracy, portability and maintenance costs. Prefer removing unnecessary work
to obscuring it with a complicated mechanism. A justified architectural constraint may prevent
an otherwise faster implementation.

Store the reproducible workload, method, raw or sufficiently summarized results and interpretation
with the subject they describe. State limits and contrary results. An inconclusive experiment is
not proof of equivalence; a local improvement is not automatically an end-to-end improvement.

Use [Verification and testing](07-verification-and-testing.md) to preserve behavior and
[Research and evidence](03-research-and-evidence.md) when interpreting external comparisons.
