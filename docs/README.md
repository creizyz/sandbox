# Sandbox documentation

Sandbox is a C++23 project for developing algorithms, data structures, and reusable library code.
A future application will use the library. [AGENTS.md](../AGENTS.md) defines the working agreement,
and the [guidelines](../guidelines/README.md) provide reusable engineering standards.

## Project layout

| Space | Responsibility | Current scope |
| --- | --- | --- |
| [`app/`](../app/) | Application entry point, integration, and application-specific behavior | Reserved for the future application |
| [`library/`](../library/) | Reusable algorithms, data structures, and utilities | The `sandbox` CMake target contains fixed-size math, handles, worker-thread scheduling, JSON and YAML I/O, binary serialization experiments, and string helpers |
| [`tests/`](../tests/) | Correctness and regression evidence | GoogleTest suites for vectors, vector views, quaternions, handles, and JSON and YAML I/O |
| [`benchmarks/`](../benchmarks/) | Performance experiments and supporting measurements | Google Benchmark microbenchmarks for vector, geometry, and document I/O operations |

The app, tests, and benchmarks consume the library. Keep the library independent of those
consumers. Keep component-specific contracts, workload descriptions, and measurements with
their owning space when they need their own reference. Use `docs/` for project-wide policies,
build instructions, and decisions that affect multiple spaces.

## Design direction

Data-oriented design is a project-wide preference. Start with the data, transformations, and
access patterns, then choose representations, ownership, and bulk operations to fit them.
Design for efficient multithreaded execution across the app and library. Look for independent
work, define how mutable data is partitioned or synchronized, and make ordering and lifetime
requirements explicit. Measure throughput, scaling, and synchronization costs where they
matter. Choose the execution model for each workload. These are design goals for the project,
and individual APIs document their own concurrency guarantees.

## Contributor references

Number documents within each section using two-digit prefixes, starting at `01`. Use
`README.md` for section indexes.

| Task | Reference |
| --- | --- |
| Write or review C++ and check API maturity | [C++ conventions and API maturity](development/01-cpp-conventions.md) |
| Read or write JSON, including comment and number handling | [JSON I/O](../library/io/docs/01-json.md) |
| Read or write portable YAML configuration documents | [YAML I/O](../library/io/docs/02-yaml.md) |
| Convert JSON or YAML values to and from core values | [Document values and core conversion](../library/io/docs/03-document-value.md) |
| Measure JSON, YAML, or core value conversion performance | [Document I/O benchmarks](../benchmarks/io/docs/01-document-performance.md) |
| Configure, build, format changed files, export compile commands, run tests, or run benchmarks | [Build and validation](development/02-build-and-validation.md) |
| Record or review an architectural decision | [Architecture decision records](adr/README.md) |
| Select an agent procedure for a recurring task | [Repository skills](#repository-skills) |

## Repository skills

Select a skill by the result the task calls for. The guidelines and project documentation
continue to define the applicable standards and contracts.

| Skill | Requested outcome |
| --- | --- |
| [`review-change`](../.agents/skills/review-change/SKILL.md) | Findings about a proposed or existing code change |
| [`investigate-defect`](../.agents/skills/investigate-defect/SKILL.md) | The cause of an observed failure or a check that distinguishes plausible causes |
| [`research-question`](../.agents/skills/research-question/SKILL.md) | A bounded technical answer supported by external evidence |
| [`design-change`](../.agents/skills/design-change/SKILL.md) | A design recommendation with alternatives and consequences |
| [`implement-change`](../.agents/skills/implement-change/SKILL.md) | A working and verified C++ feature, fix, or refactor |
| [`benchmark-change`](../.agents/skills/benchmark-change/SKILL.md) | A controlled performance measurement or comparison |
| [`maintain-documentation`](../.agents/skills/maintain-documentation/SKILL.md) | A durable update to repository documentation or its indexes |
