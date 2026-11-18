---
name: design-change
description: Compare approaches and propose a decision for a component boundary, data representation, public API, or concurrency model. Use when the requested outcome is a design or trade-off analysis.
---

# Design a change

1. Read the [project context](../../../docs/README.md) and the affected contracts. Identify the
   consumers, data transformations, access patterns, and invariants that drive the decision.
2. Compare the simplest adequate design with plausible alternatives. For each material option,
   trace ownership, mutation, allocation, invalidation, and failure behavior. For parallel work,
   identify independent work and shared data. Use the
   [architecture](../../../guidelines/04-architecture-and-design.md) and
   [interfaces](../../../guidelines/06-interfaces-and-contracts.md) guidelines.
3. Recommend an option with its consequences, compatibility implications, and the test or
   measurement that would resolve important uncertainty. Record an ADR when requested and
   follow the [ADR index](../../../docs/adr/README.md).
