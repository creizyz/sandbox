# Change management

Use these standards when scoping, reviewing, migrating, integrating or releasing changes.
The bootstrap working agreement and project policies define authorization; this guideline does
not grant permission to commit, publish, deploy or modify external state.

## Scope and reviewability

Define the intended outcome, affected contracts and completion evidence. Keep a change coherent
and avoid unrelated cleanup. A necessary prerequisite belongs in scope only when authorized;
otherwise explain the dependency and seek the missing decision. Preserve unrelated work and
identify existing failures without attributing them to the new change without evidence.

Scale planning and review to complexity, reversibility and consequence. A small correction need
not produce a formal design record. A new architecture, persistent-data migration or public
compatibility change needs its relevant consumers, alternatives and failure modes understood
before implementation.

Prefer changes a reviewer can evaluate from the final artifacts and evidence. Explain the concrete
problem, resulting behavior, rationale and validation. Do not rely on conversational history or
report abandoned approaches unless they explain a material trade-off.

## Compatibility and migration

Identify consumers and the compatibility obligations in
[Interfaces and contracts](06-interfaces-and-contracts.md). For migrations, define supported
transitional states, conversion ownership, validation, rollback limits and removal conditions.
Avoid an indefinite period with two authoritative definitions.

For dependencies, configuration and generated artifacts, record provenance and the intended source
of truth. Regeneration must use documented inputs and tools. Do not silently edit generated output
in place of its source, and do not assume a generated change is safe without reviewing its effect.

Approval applies to the decision and scope actually approved. New information may require a revised
proposal; it does not permit inventing approval or rewriting history. Separate technical review from
permission to perform an external or irreversible action.

## Completion and integration

Use the applicable [verification](07-verification-and-testing.md) and project gates. Keep tests,
contracts, user-facing behavior and documentation consistent. Report checks performed, outcomes,
skips and remaining limitations. Do not conceal a validation gap behind a successful unrelated test.

Keep commits and review units focused when committing is authorized. Explain the behavior change
and why it is needed, particularly when reference outputs or compatibility commitments change.
Exclude unrelated generated files, local caches, credentials and build products.

For release or deployment work, define readiness, monitoring, rollback and recovery according to
actual consequences. Demonstrate the assumptions required for rollback; reversing code does not
necessarily reverse a data conversion or an external effect. Stop at the authorized boundary.
