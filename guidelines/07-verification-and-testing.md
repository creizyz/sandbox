# Verification and testing

Use these standards to select and interpret evidence of correctness. Project documentation owns
the concrete commands, required gates, environments and acceptance criteria. A skill may organize
execution but must not weaken those requirements.

## Evidence follows behavior and risk

Identify the behavior, invariant or failure mode to establish. Choose evidence capable of detecting
a plausible defect, preferably independent of the implementation strategy. Tests that duplicate
the implementation or assert incidental internal structure provide weak protection against a
shared mistake.

For behavior changes and bug fixes, provide suitable regression evidence. A reproducer should fail
for the defect for the intended reason when practical. Pure documentation or mechanical changes
need checks suited to their effects rather than unrelated tests written to satisfy a ritual.
Risk and project policy determine whether broader integration, sanitizer, visual, security or
system evidence is required.

Exercise relevant boundaries, invalid inputs, failure paths and state transitions as well as the
normal case. Use the smallest representative fixtures that preserve the behavior under test.
Prefer public contracts to private layout details unless the internal invariant itself is the
subject of verification.

Verify reusable APIs through representative caller code. For public templates or header-only
APIs, instantiate the operations under test. Exercise types, sizes and build configurations that
select materially different implementations or checks.

## Isolation and repeatability

Control time, randomness, external services and environmental inputs where they affect outcomes.
Avoid sleep-based synchronization and dependence on machine speed. When nondeterminism is part
of the system, state the acceptance model and preserve sufficient observations to investigate a
failure. Record seeds, inputs and relevant versions for generated cases.

When randomized tests use a reference model, have it predict legal and rejected operations
independently. Check both the result and the state after rejection.

Keep fixtures isolated and give side effects explicit owners and cleanup. Fakes establish behavior
within their model; they do not prove integration with the real dependency. Use real integration
tests when its semantics are material, without requiring expensive dependencies for unrelated tests.

Choose numerical tolerances from the model and expected error, not from a failing result.
Golden references encode intended behavior: change them only for an understood, authorized
behavior change. Do not suppress a diagnostic, delete coverage or loosen a threshold merely to
obtain a passing run.

## Interpretation and reporting

Run focused checks while developing, then the required project gates before completion. Repeat
or broaden checks when new changes, failures or unresolved concerns justify it. Preserve the
distinction between a baseline failure and a regression caused by the change; neither permits a
false claim of a fully passing result.

Report what ran, under which relevant conditions, and its outcome. A skipped, unavailable or
unexecuted check is a validation gap. A non-rendering test cannot prove pixels correct; a screenshot
cannot prove interaction; one successful execution cannot establish all supported conditions.

Review evidence quality as well as pass/fail counts. Coverage metrics and static analysis are
signals with limitations. Separate observed results from the scope of the conclusion they support.
For quantitative experiments, use [Performance and measurement](08-performance-and-measurement.md).
