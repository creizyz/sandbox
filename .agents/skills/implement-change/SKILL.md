---
name: implement-change
description: Complete an authorized Sandbox C++ feature, fix, or refactor through affected callers and relevant validation. Use when the requested outcome is a working code change.
---

# Implement a change

1. Identify the affected entry point and caller-visible contract. Consult the
   [C++ conventions](../../../docs/development/01-cpp-conventions.md) for style and API maturity.
   Check sample callers, older inputs, and stable contract obligations as applicable.
2. Make the smallest coherent change through the normal entry point. Follow the
   [programming guideline](../../../guidelines/05-programming.md) and keep behavior consistent
   across applicable types, sizes, and failure paths.
3. Use [build and validation](../../../docs/development/02-build-and-validation.md) to select
   checks that exercise the change. Instantiate altered template operations from caller code when
   the regular build misses them. Disable build auto-formatting when it would touch unrelated edits.
4. Review the final diff and report the behavior change, compatibility effects, and validation
   actually performed.
