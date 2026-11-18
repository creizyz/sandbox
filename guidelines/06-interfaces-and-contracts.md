# Interfaces and contracts

Use these standards for APIs, configuration, file formats, protocols and other boundaries between
producers and consumers. Project documentation owns their concrete definitions and compatibility
commitments; implementation comments may state local obligations at the declaration.

## Contract content

Specify accepted inputs, units, ranges, defaults, output meaning and failure behavior. Identify
preconditions, postconditions and invariants where they remove ambiguity. Separate a caller
obligation from a condition the implementation validates; do not imply protection that is absent.

Describe ownership, borrowing, lifetime and invalidation of returned or retained data. A view needs
an explicit relationship to its backing storage. Define identity across removal, replacement and
reordering. State whether operations mutate state, block, allocate, invoke callbacks or retain
arguments when those effects constrain correct use.

For stateful and asynchronous interfaces, define legal transitions, ordering, reentrancy,
concurrency, completion, cancellation and partial success. State whether retries are safe and
how duplicate operations are recognized. Do not promise transactional behavior or exactly-once
effects without a mechanism that establishes them.

## Simplicity and representation

Expose the domain operations consumers need while hiding replaceable implementation decisions.
Use types to distinguish alternatives and invalid states where practical. Keep data representations
and validation rules consistent across consumers; avoid parallel definitions that must change
together. Public structs can be appropriate for plain values; an invariant-bearing object needs
controlled mutation.

For serialized data, specify encoding, byte order where relevant, lengths, limits, optional fields,
unknown-field behavior and version handling. Parse defensively at trust boundaries. A successful
decode does not establish semantic validity or permission to act on its contents.

## Evolution and evidence

Identify consumers before changing observable behavior. Compatibility includes meanings, defaults,
errors, timing and resource obligations, not just signatures or field names. Mark implementation
details explicitly rather than assuming callers will know which behavior is stable.

Prefer a bounded migration with clear old/new behavior over indefinite duplicate interfaces.
Document deprecation, conversion, rollback limits and removal conditions where required. Changes
to accepted contracts need the applicable authorization; tests alone cannot approve a new contract.

Verify boundary behavior, including invalid inputs and lifecycle transitions, through
[appropriate evidence](07-verification-and-testing.md). For failures and retries, also consult
[Reliability and diagnostics](10-reliability-and-diagnostics.md).
