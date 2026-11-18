# Organizing an AI-assisted project

## Purpose and scope

Organize agent instructions and documentation so that a new agent can establish the working
agreement, select a suitable methodology and retrieve the context needed for its task. Keep
reusable guidance independent of changing project details, with explicit ownership and traceable
sources of authority.

This guideline records the responsibilities, rationale, trade-offs and evaluation criteria for
organizing AI-assisted projects. Reusable standards live in root `guidelines/`. The project
documentation index describes where project-specific material belongs.

The organization is an engineering design choice, not a universal requirement of `AGENTS.md` or
an empirically established optimum for every agent. Validate its effectiveness with actual tasks.

## 1. Separate four responsibilities

| Layer | Question answered | Owns |
| --- | --- | --- |
| Bootstrap | How do we collaborate, and where do I start? | Working agreement, authorization boundaries, clarification and communication expectations, discovery procedure and entry points |
| Skills | How should I conduct this kind of task? | Task selection criteria, methodologies, evidence collection, supporting tools and expected deliverables |
| General guidelines | What standards should the work satisfy? | Coding and design principles, research standards, documentation conventions and criteria for judging results |
| Project documentation | What is required, decided or implemented here? | Domain concepts, architecture, folder structure, module contracts, commands, configuration, asset pipelines and accepted decisions |

Classify a statement by its responsibility and scope. Give it one authoritative home; other layers
link to that home rather than repeating the statement. A document can explain a related concept
without becoming a second owner of its rules.

For example, a research guideline defines acceptable evidence and how to express uncertainty.
A research skill describes how to formulate a question, retrieve and compare sources, and produce
an assessment. Project documentation records the investigation and any decisions adopted from it.

For performance work, a general guideline requires reproducible measurements. A benchmarking skill
describes the experimental procedure and interpretation. Project documentation supplies actual
commands, representative workloads and acceptance thresholds.

## 2. Bootstrap: a working agreement and discovery entry point

Keep the root `AGENTS.md` short enough to load for every task. Its primary responsibilities are:

- Explain how the user expects to collaborate: analysis versus implementation, permission to edit,
  when clarification is necessary, expected initiative, reporting and completion boundaries.
- Explain how to find applicable skills, general guidelines and project documentation.

Make these instructions reusable across projects with minimal changes to entry-point links.
Put detailed module descriptions, capability lists, build commands and changing implementation
facts in indexed project documentation. Put extended communication and reasoning standards in
their authoritative guideline; this project uses
[Reasoning and communication](02-reasoning-and-communication.md).

`AGENTS.md` can technically contain project knowledge. Reserving it primarily for collaboration
and discovery is our organizational choice. A brief mandatory warning or routing instruction is
justified when an agent could otherwise act before finding a consequential constraint. Prefer
“consult the data-handling policy before accessing production data” to copying the policy.

Describe authorization in terms of user intent and scope. Distinguish requests for analysis from
requests to implement an outcome. Requiring users to name every necessary file is a separate,
more restrictive collaboration policy, not a consequence of this documentation structure. Do not
silently relax an explicitly chosen policy. Ask for missing information when it materially affects
correctness, scope or authorization; state reasonable assumptions for lesser uncertainty.

The discovery procedure should direct an agent to:

1. Identify the requested task, affected subjects and authorized actions.
2. Consult the relevant indexes and skill descriptions.
3. Load applicable standards and contracts before making decisions they govern.
4. Inspect implementation and evidence as needed, expanding context when dependencies or
   uncertainty require it.
5. Surface material contradictions instead of silently choosing a convenient interpretation.

Do not require recursive reading of every linked document. Conditional discovery should reduce
irrelevant reading without making essential constraints optional.

## 3. Skills: reusable methodologies with explicit scope

Use skills for recurring tasks where an explicit methodology improves outcomes: review,
investigation, research, benchmarking or implementation. Describe when a skill applies, relevant
exclusions, its procedure, required evidence and expected output. Scale the procedure to the task.

A portable debugging skill can require reproduction, competing hypotheses, discriminating
observations and regression verification. It obtains project-specific commands and constraints
from project documentation rather than embedding them. A skill must not invent authorization or
change a project contract simply to complete its procedure.

Allow specialization when the evidence or tools differ materially. Use focused skills or
supporting references where useful. Reserve skills for workflows that benefit from a recurring
methodology, and keep project-specific procedures focused on their actual scope.

Keep the skill entry point concise. Load supporting references, scripts and templates only when
needed. Define narrow selection descriptions so unrelated tasks do not activate a large workflow.
Skills reference general standards and discover project details through documented entry points.
They should not duplicate the documentation hierarchy or require a separate configuration framework
without demonstrated need.

Review skills when models, tools or observed behavior change. A methodology can become obsolete
even while the project contracts it uses remain stable.

## 4. General guidelines: standards independent of one implementation

General guidelines describe how to judge work across projects: design quality, coding principles,
evidence quality, research rigor and documentation practices. They remain useful to human
contributors as well as agents. Their authority does not depend on a skill being activated.

Keep project bindings explicit. Project documentation owns concrete commands, workloads and
thresholds. Distinguish a language-specific convention from a principle that applies across
languages.

Permit documented project specializations and justified exceptions where the general rule allows
them. Record the scope, rationale and approving authority of an exception. Collaboration and
authorization boundaries remain separately governed; a local technical document must not
accidentally override them. Repository guidance also remains subject to the agent host's
higher-priority instructions and permission controls.

## 5. Project documentation: local ownership with a global index

Place documentation near the code or assets it describes. Module contracts, implementation notes,
formats, measurements and component-specific decisions should have an identifiable local owner.
Asset provenance, licensing and generation procedures belong near the relevant asset collection
when practical. Update descriptions alongside changes to their subjects.

Retain a project-wide documentation home for subjects without a single local owner: system
architecture, integration, end-to-end workflows, development environment, deployment and decisions
spanning components. Do not assign a cross-cutting document to an arbitrary module merely to keep
all project documentation below source directories.

The top-level index links to local indexes rather than copying their contents. An index entry
should state what question a page answers and when it applies.

A filename inventory alone is insufficient for reliable selection. Keep index descriptions short
and maintain them when scope changes. Avoid chains of empty indexes that add navigation without
helping selection.

Distinguish accepted requirements, current implementation, proposals, research and historical
material. Code and observations establish current behavior; accepted contracts establish intended
behavior. Neither automatically settles a disagreement with the other. Report a mismatch and
determine whether the task authorizes changing the implementation or the contract. A proposal is
not an accepted requirement, and a specification is not proof that a feature is implemented.

## 6. Reflect responsibility in filesystem placement

Keep the bootstrap entry point, skills and general guidelines accessible from the repository root.
Keep project-specific documentation beside its subject, with a root documentation area for
cross-cutting concerns. Consult the [project index](../docs/README.md#project-layout) for the
repository's concrete layout and the [documentation standard](11-documentation-and-knowledge.md)
for authoring and maintenance.

Preserve host-recognized entry points where practical. Codex discovers `AGENTS.md` and repository
skills in `.agents/skills/` according to its own scope and startup rules; arbitrary replacement
folders are not automatically equivalent. Other agents may need explicit configuration or routing.
Test discovery in the actual host and launch directory instead of assuming identical behavior
across tools. Avoid maintaining independently edited copies solely for different hosts.

## 7. Stability, dependencies and distribution

The expected order from most to least frequent change is project documentation, general
guidelines, skills, then bootstrap instructions. Treat this as a design expectation, not an
invariant or a classification algorithm. A mature module contract can outlive a skill; a new tool
can require immediate methodology changes. Responsibility, scope and ownership determine placement.

Stability depends on references as well as folders. Bootstrap instructions should link to stable
entry points. Skills should obtain changing commands and constraints from project documentation.
Local documents should describe their own subjects. Frequently editing bootstrap instructions to
track implementation changes is evidence that the boundary needs review.

Reusing guidance across projects introduces distribution choices. Checked-in copies are available
and reproducible but can diverge. Shared personal installations reduce duplication but make
behavior depend on machine state and can change older projects unexpectedly. For a small number
of projects, copies with recorded provenance may suffice. Introduce versioned distribution when
its benefits justify the maintenance cost. Preserve enough provenance to identify which guidance
governed a change; avoid building a management system before that need exists.

## 8. Benefits, limits and evaluation

| Benefit | Corresponding limit | Design response |
| --- | --- | --- |
| Small, portable startup context | Essential constraints can remain undiscovered | Make consequential routing mandatory before the affected actions |
| Targeted context loading | Poor descriptions or excessive indirection can misroute agents | Use task-oriented indexes and inspect actual retrieval behavior |
| Reusable methodologies | Generic recipes can miss domain-specific evidence or become obsolete | Specialize where justified and evaluate after tool/model changes |
| Documentation near its subject | Cross-cutting subjects have no single local owner | Retain project-wide documentation and link local owners |
| Independent standards and project facts | Duplicate ownership or ambiguous exceptions can create conflicts | Give statements one authoritative home and define exception scope |
| Shared guidance across repositories | Copies drift; external installations weaken reproducibility | Record provenance and adopt versioned distribution when justified |

This organization improves maintainability only if agents select and apply the right information.
Link validation establishes that references resolve; it does not establish relevance, correct
authority, truth or compliance. Text instructions do not replace executable checks or permission
controls. Keep mechanical formatting and validation rules enforced by tools where possible.

Evaluate the organization with representative tasks: a read-only review, a small implementation,
a cross-module change and a research question. Include cases where no skill should activate and
cases where explicit approval is required. Observe whether the agent:

- Finds the relevant standards and contracts before taking dependent actions.
- Selects an appropriate skill and avoids activating unrelated methodologies.
- Distinguishes intended, implemented, proposed and historical behavior.
- Respects authorization and asks for clarification at the intended boundaries.
- Produces the required evidence and completes the authorized scope.
- Avoids unnecessary reading, commands and repeated requests for information.

Use observed failures to adjust routing, ownership or methodology. A cleaner directory tree alone
is insufficient evidence of better agent performance. Retest important cases after significant
guidance, host or model changes; scale evaluation effort to the consequences of failure.

## References

The following primary sources support host behavior and vendor recommendations. They do not
establish this entire organization as a universal standard. Consulted 17 November 2026:

- [Custom instructions with AGENTS.md](https://learn.chatgpt.com/docs/agent-configuration/agents-md):
  working agreements, project instructions, discovery and scope.
- [Build skills](https://learn.chatgpt.com/docs/build-skills): progressive disclosure, activation
  descriptions and repository skill locations.
- [Rethinking skills and prompts](https://developers.openai.com/blog/rethinking-skills-and-prompts-for-gpt-6-astra):
  conditional documentation lookup, concise skill routing and revisiting accumulated instructions.
- [Testing Agent Skills Systematically with Evals](https://developers.openai.com/blog/eval-skills):
  evaluating activation, outcomes, process and efficiency with representative tasks.
