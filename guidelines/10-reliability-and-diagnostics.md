# Reliability and diagnostics

Use these standards for behavior under failure, interruption, overload and environmental change.
Project contracts define required availability, durability, recovery times and permitted loss.

## Failure semantics

Distinguish invalid requests, expected rejection, unavailable dependencies and programmer defects.
Specify what state and external effects remain after failure. An operation that can partially
succeed must expose enough information for a caller to recover without guessing. Do not promise
atomicity merely because one API call initiates several effects.

Give resources an owner and cleanup path across success, failure and cancellation. Determine when
an object is ready for use and when it can be destroyed. In asynchronous work, completion of
submission is not necessarily completion of use; resource retirement must follow the applicable
completion contract.

## Recovery, load and cancellation

Bound waits and work where the environment can stall or overload. Define timeout and cancellation
semantics, including whether an effect may still complete after the caller stops waiting.
Cancellation must preserve invariants and must not be reported as proof that remote work was undone.

Retry only failures for which repetition is meaningful and safe. Define attempt or time limits,
backoff where appropriate, idempotency and duplicate handling. A retry that hides a permanent error
or amplifies overload is not recovery. Make fallback behavior explicit; silent degraded results
can violate a contract even if execution continues.

Define queue limits, backpressure and rejection policy for sustained load when relevant. Distinguish
transient recovery from data repair. Restoration and rollback claims need evidence that covers
state, schema and external effects, not just process restart.

## Diagnostic quality

Preserve useful causes and context at the boundary that can explain or recover from failure.
Identify the operation and relevant resource, stage, version or correlation identity without
exposing secrets. Messages should help distinguish likely causes and next investigations.

Choose logs, metrics and traces according to the questions they answer. Avoid duplicate reporting
at every layer, unbounded event volume and high-cardinality labels without a budget. Instrumentation
has performance, storage and privacy costs; it needs ownership and retention rules where applicable.

Test meaningful failure paths, cleanup and recovery under representative conditions. A simulated
failure establishes behavior under that model, not proof of production resilience. Report known
limits using [verification standards](07-verification-and-testing.md), and assess exposure using
[Security and trust](09-security-and-trust.md).
