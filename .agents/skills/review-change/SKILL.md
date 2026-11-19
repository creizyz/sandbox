---
name: review-change
description: Review a proposed or existing code change for regressions, contract breaks, and missing verification. Use for reviews of diffs or patches when the requested outcome is findings.
---

# Review a change

1. Identify the diff and intended behavior. Read affected contracts and the relevant
   [verification guideline](../../../guidelines/07-verification-and-testing.md).
2. Trace changes through callers, including older inputs, failure paths, and configuration or
   template branches that the diff affects. Check whether the test would fail for the defect
   it claims to catch.
3. For each candidate finding, identify a triggering condition, affected caller, observable
   consequence, and evidence connecting them to the change. Attribute baseline defects separately.
   If running an authorized build, follow [build and validation](../../../docs/development/02-build-and-validation.md)
   with automatic source formatting disabled for the review. Check the work summary for the
   **Stable** statistics and per-case **Slower** and **Faster** results. See
   [Benchmark changed files](../../../docs/development/02-build-and-validation.md#benchmark-changed-files).
4. Report actionable findings in priority order with file locations. If there are none, state
   what you reviewed and any meaningful verification gap.
