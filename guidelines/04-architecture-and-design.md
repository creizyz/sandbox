# Architecture and design

Use these criteria when assigning responsibilities, changing dependencies or evaluating structural
alternatives. This guideline defines design standards; project documentation defines the actual
components, deployment, dependency graph and accepted architecture.

## Preferred approach: data-oriented design

Prefer data-oriented design. Start from the data, its transformations and actual access patterns,
then design ownership, layout and bulk operations around them. Choose representations from those
requirements and measurements.

Build zero-cost abstractions over that foundation for usability. Typed handles, non-owning object
views and domain operations should make the data convenient and safe to use while preserving its
layout, locality and efficient bulk processing. The owning module controls storage; callers should
not need to understand its implementation to use the interface correctly.

Zero cost means no additional runtime work, allocation or storage beyond an equivalent direct
implementation with the same semantics and safety guarantees. Optional features should impose no
runtime work, allocation or per-object storage when unused. An interface must not introduce a
second state model, materialize temporary objects or force repeated lookups merely to provide
object-like syntax. It may expose individual operations without forcing bulk algorithms through
per-object calls.

Keep view lifetimes, invalidation, identity and mutation contracts explicit. Zero-cost goals do
not justify unsafe borrowing or unchecked identities. Assess generated code and measure affected
hot paths when cost is uncertain; abstraction syntax alone does not establish zero overhead.
Justify unavoidable overhead against the required behavior and simpler alternatives. Account for
conceptual, implementation and compilation costs separately from runtime cost.

## Responsibilities and information hiding

Assign a cohesive responsibility to each module. Hide decisions that callers should not need to
understand, especially storage, algorithms, resource lifetimes and replaceable implementation
choices. Prefer deep modules: a small, comprehensible interface should provide substantial useful
behavior. Judge interface size by the concepts and obligations imposed on callers, not only by
method count.

Keep domain rules separate from presentation and execution mechanisms where their responsibilities
differ. Represent ownership, state transitions and effects explicitly. An adapter should translate
between boundaries rather than become a second owner of either domain's rules.

Do not split a cohesive responsibility merely to create more layers or files. Conversely, physical
proximity does not justify coupling unrelated concerns. Component names, library targets and
deployment units need not be identical; document the boundaries that actually matter.

## Dependencies and abstractions

Make dependency direction intentional and enforceable where practical. Keep foundational modules
independent of their consumers. Contain external libraries behind boundaries when their types,
lifecycles or instability would otherwise spread through unrelated code. Do not add wrappers that
merely rename a stable API without hiding a consequential decision.

An abstraction must justify its additional concepts, indirection, maintenance and operational
cost. Prefer removing duplicated decisions to building a framework for managing them. Introduce
extension points for demonstrated variation or a concrete requirement, rather than hypothetical
future consumers. A simpler direct implementation is a valid alternative.

Dependencies are design commitments. Assess their capabilities, integration, maintenance, trust,
licensing and removal cost. A third-party solution and an internal implementation both require
justification; neither is preferable solely because of its origin.

## Data, state and time

Identify the owner of each mutable state and resource, the readers, and permitted transitions.
Make aliasing, invalidation and synchronization obligations visible alongside the data and view
contracts above.

Evaluate concurrency, distribution and caching against concrete requirements. They add ordering,
consistency and failure questions that must have explicit answers. Preserve deterministic behavior
where required, and identify the inputs, ordering and numerical assumptions on which it depends.

For parallel work, define the units that can execute independently and how they access shared
state. Measure scaling with representative workloads and account for scheduling overhead,
synchronization, contention and memory bandwidth.

Treat latency, throughput, memory and startup constraints as architectural inputs when applicable.
Define budgets and obtain evidence through [performance measurement](08-performance-and-measurement.md).
Do not require every system to satisfy an unmeasured real-time or zero-allocation constraint.

## Design assessment and evolution

Compare the simplest adequate design with plausible alternatives, including keeping the current
design. Assess correctness, conceptual load, change propagation, performance, testability,
operability and migration cost. State assumptions and failure modes. Scale design records to the
consequences and reversibility of the decision.

Record significant decisions with context, considered alternatives, rationale, consequences and
status. Distinguish proposed architecture from accepted policy and implemented capability.
Approval of a design does not prove its implementation correct or waive unrelated constraints.

Preserve [contracts](06-interfaces-and-contracts.md) during evolution or plan an explicit migration.
Make the final structure explainable without relying on conversational history. Review the
resulting boundaries, not merely whether the implementation follows the initial diagram.
