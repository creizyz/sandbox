# Programming

Use these standards when implementing or reviewing code. They express reusable implementation
criteria. The project selects languages, versions, conventions, tools and domain-specific limits.
Follow its applicable language rules; established code is evidence of practice, not proof that a
known defect or obsolete convention should be copied.

## Clarity and scope

Implement the requested behavior with the smallest coherent change. Prefer direct control flow,
domain vocabulary and explicit dependencies. Names should identify purpose, quantities and units;
avoid generic roles that conceal responsibility. Keep unrelated cleanup separate unless necessary
for the authorized change.

Functions should have a coherent responsibility and an understandable contract. Split code when
it hides a meaningful decision or reduces complexity, not to satisfy an arbitrary line or parameter
limit. Introduce options objects when they group a coherent concept, not to conceal unrelated inputs.

Use the project's formatter and static checks. Keep mechanical conventions in executable
configuration where possible. Comments explain rationale, constraints and non-obvious consequences;
declarations document caller obligations. Avoid comments that merely repeat the statements below.

## Types, state and identity

Express invariants in types and construction where practical. Prefer explicit alternatives over
independent flags that permit invalid combinations. Use distinct identities and units when mixing
them would be consequential. Specify overflow, precision, range and conversion behavior at numeric
boundaries; do not treat a cast as validation.

Distinguish identity from a position in storage that may move. Validate handles when objects can
be removed or replaced. Initialize state deliberately and preserve invariants on every supported
exit path, including failure and cancellation.

## Ownership, mutation and lifetime

Make ownership and borrowing explicit. Prefer values and scoped resource ownership where
appropriate; use sharing when ownership is genuinely shared. Tie acquisition and release together
through the language's resource-management mechanisms. Document the lifetime and invalidation of
references, views, iterators and callbacks retained across operations.

Expose read-only access deliberately and restrict mutation to the appropriate interface. In
languages with const-qualified access, distinguish a fixed handle from a protected pointee and
logical constness from physical mutation. Read-only access does not establish thread safety or
exclude mutation through another alias. Do not defeat a contract with casts or hidden mutation.
Prefer const-qualified observers and values that do not change where the language supports them.
Preserve value and move semantics: mechanically making every member or returned value const can
prevent valid assignment or efficient transfer. Borrow rather than copy only when the lifetime
and access cost make borrowing the correct interface.

Avoid ambient mutable state that obscures dependencies, isolation or ordering. A cache or shared
service needs an explicit owner, lifecycle, synchronization policy and invalidation model. Do not
replace globals with a generic service locator that preserves the same hidden dependencies.

## Errors and boundaries

Validate untrusted values before using them to allocate, index, compute sizes or change state.
Represent expected rejection separately from programmer defects and environmental failure.
Use the language and project's error conventions consistently; do not assume exceptions, return
values or assertions are interchangeable. Assertions cannot be the only validation of external
input when production builds may omit them.

Preserve the cause and useful context when propagating failures. Catch errors where recovery or
context can be added; avoid silent loss, half-initialized objects and ambiguous partial success.
Follow [reliability standards](10-reliability-and-diagnostics.md) for recovery semantics.

## Cost and dependencies

Prefer storage and bulk operations that match access patterns. Lightweight views can make
data-oriented storage convenient without duplicating it into a second object model. Preserve
efficient iteration and avoid unnecessary copying or allocation where costs matter.

Aim for abstractions whose cost is no greater than an equivalent direct implementation, and for
optional features that impose no runtime work, allocation or per-object storage when unused.
Account for unavoidable costs explicitly. These goals do not eliminate compilation complexity
or justify unsafe lifetime assumptions. Measure affected hot paths rather than asserting that a
particular container or abstraction is universally fast.

Include or import what is used and respect dependency boundaries. Isolate implementation details
that would unnecessarily couple consumers. Use language features and dependencies to simplify
real requirements; do not introduce machinery solely because it is available.

## Verification and maintenance

Provide evidence for behavior changes and regressions according to
[verification standards](07-verification-and-testing.md) and the project validation policy.
Address diagnostics in owned code rather than suppressing them to obtain a clean result.
Any justified suppression needs a narrow scope and explanation.

Keep tests, contracts and explanatory documentation consistent with intended behavior.
Remove obsolete paths when the change authorizes their removal; record concrete follow-ups
instead of leaving unexplained commented-out code or unowned work markers.
