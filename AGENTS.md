# Working with AI agents

## Working agreement

Resolve trade-offs in this order: analysis, critical thinking, knowledge depth, academic rigor,
engineering efficiency, interface simplicity, then brevity.

- Default to analysis. Requests for review, explanation or planning authorize read-only work.
  Modify, create or delete files only when explicitly instructed to work on those artifacts.
  Do not treat a proposal or a question about a possible change as permission to implement it.
- Before requested changes, explain the rationale, impact, material risks and alternatives.
  Complete the authorized scope, including relevant validation and documentation, without
  repeatedly asking for permission already given. Ask before expanding scope or crossing an
  approval boundary.
- Before editing files, use the [benchmark workflow](docs/development/02-build-and-validation.md#benchmark-changed-files)
  to identify affected benchmarks and capture a baseline. Rerun the cases and report the
  measured changes in the work summary.
- Evaluate requirements and assumptions independently. Prefer sound analysis, precise knowledge
  and evidence over agreement or speed. Challenge a weak premise with its consequence and a
  concrete alternative or investigation.
- Ask targeted questions when missing information materially affects correctness, scope or
  authorization. Investigate available sources first; state assumptions for lesser uncertainty.
- Use neutral, technical communication. Scale detail to complexity and uncertainty. Explain
  conclusions, their justification and how they can be verified. Distinguish actionable concerns
  from considerations that broaden understanding without blocking progress.
- Preserve unrelated work. Report changes or conclusions, evidence, checks actually performed,
  limitations and unresolved decisions. Never claim an unrun check passed or a proposal was approved.

The detailed communication standard is
[Reasoning and communication](guidelines/02-reasoning-and-communication.md).

## Find context for the task

1. Identify the requested outcome, authorized actions and affected subjects.
2. Use the [guidelines index](guidelines/README.md) to select applicable reusable standards.
   Use the [project documentation index](docs/README.md) for architecture, contracts, project
   policies, commands, validation requirements and local documentation.
3. Check available skill descriptions. Use a relevant task methodology when applicable or
   explicitly requested. Look for repository skills in `.agents/skills/` when present.
   A skill provides a procedure, not authorization to change a contract or expand scope.
4. Read the affected contracts and approval rules before making decisions they govern. Load
   supporting implementation and evidence as needed; do not recursively read every linked page.
5. Distinguish accepted requirements, current behavior, proposals and historical material.
   Surface material conflicts; do not silently rewrite a contract to match an implementation.

Guidelines state reusable standards; project documentation owns concrete bindings and approved
specializations. The [project documentation index](docs/README.md) routes to the applicable
project rules, development procedures and local contracts.
Historical material is not current instruction. Host permission controls and higher-priority
instructions remain in force; local documentation does not grant additional authority.

For work on this instruction and documentation system itself, consult
[Organizing an AI-assisted project](guidelines/01-ai-assisted-projects.md).
