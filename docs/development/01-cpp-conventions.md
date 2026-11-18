# C++ conventions and API maturity

These conventions apply to C++ in `app/`, `library/`, `tests/`, and `benchmarks/`. Use C++23 and
the [programming guideline](../../guidelines/05-programming.md).

## House style

- Use `PascalCase` for types and `snake_case` for functions, methods, parameters, and local variables.
- Prefix instance data members with `m_` and use `snake_case` after the prefix.
- Indent with four spaces and put opening braces for declarations, function bodies, and control
  blocks on their own lines.
- Use the library include root for library headers in consumers, for example
  `#include "math/vectors.hpp"`. Include the headers needed by the code.

Follow the required spelling for framework hooks and standard-library customization points.
Review the impact on callers when renaming a public symbol.

## Library API maturity

Record maturity for individual APIs or clearly named API groups. Update the status as the API and
its evidence evolve.

| Status | Meaning for callers |
| --- | --- |
| **Stable** | Callers can rely on the documented contract. Review compatibility with affected consumers before changing it. |
| **Experimental** | The interface or behavior is still evolving. Review changes against current consumers and update them as needed. |
| **Unclassified** | A stability decision is pending. Consult the owning API's documentation before relying on compatibility. |

`Matrix`, `VectorSoa`, quaternion `slerp`, and the `Value` serialization and JSON parsing APIs
are experimental. Stable designations are pending review of individual contracts and their
verification. Coverage informs that review. Record status changes alongside the affected contract.

Describe caller obligations where they affect correct use, including borrowing and invalidation,
thread safety, failure behavior, and numerical edge cases. See the
[interfaces and contracts guideline](../../guidelines/06-interfaces-and-contracts.md).
