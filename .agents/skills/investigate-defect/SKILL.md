---
name: investigate-defect
description: Diagnose a specific failure or unexpected behavior by isolating its trigger and testing competing explanations. Use when the requested outcome is a root cause or a discriminating next check.
---

# Investigate a defect

1. State the expected and observed behavior. Record the smallest known failing input, command,
   revision, build configuration, and seed where relevant. Consult
   [build and validation](../../../docs/development/02-build-and-validation.md) for local entry points.
2. Trace the failure through the earliest boundary at which the evidence diverges. Ask whether
   the implementation, test model, or environment made the unexpected decision.
3. Choose a check whose outcomes distinguish plausible causes. Use available observations first.
   When an authorized check requires building, disable automatic formatting if it would alter the
   evidence. Keep the original failure available for comparison.
4. Report the trigger, evidenced cause, and what a fix would need to verify. Mark remaining
   hypotheses and any gaps in reproduction. See [research and evidence](../../../guidelines/03-research-and-evidence.md).
