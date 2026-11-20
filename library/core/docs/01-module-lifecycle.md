# Worker module lifecycle

Implement the hooks on `IModule` and transfer the implementation to a
`ModuleRunner`. Existing subclasses can keep their `step` and lifecycle hook
overrides. Move execution settings out of the base constructor and invoke
`init`, `start`, and other controls on the runner. The runner owns the worker
thread and joins it before releasing and destroying the implementation.

```cpp
class Integrator final : public IModule
{
    void step(std::chrono::duration<double> elapsed) override
    {
        m_elapsed += elapsed;
    }

    std::chrono::duration<double> m_elapsed{};
};

ModuleRunner integrator(std::make_unique<Integrator>(),
                        ModuleRunner::ExecutionMode::FIXED_RATE, 60.0);
if (integrator.init() && integrator.start())
{
    // The worker runs concurrently until paused, stopped, or destroyed.
}
```

A newly created runner needs `init()` before its first `start()`.
`pause()` waits for an active step to finish and calls `on_pause()`.
`start()` on a paused module calls `on_resume()`. `stop()` accepts both
running and paused modules. A single `ONCE` step stops automatically.
From stopped, `reset()` prepares another start and `release()` returns to
the initial state. A request for the current state succeeds immediately.
An unsupported transition returns false.

The runner serializes lifecycle controls and synchronizes them with `step`.
A hook can call `is_running()` or `set_execution_mode()` on its runner.
Call lifecycle controls and destroy the runner from outside its hooks.
Lifecycle callbacks see the source state. Returning false or throwing leaves
that state intact. Exceptions from explicit lifecycle calls reach the caller.
During destruction the runner completes cleanup even when a stop or release
hook fails. Use explicit stop and release when those errors need reporting.
A `step` implementation must handle its own exceptions. As with an ordinary
`std::thread` entry point, an exception escaping a step terminates the process.

`MAX_RATE` runs successive steps immediately. `FIXED_RATE` starts the next
step after the configured period or immediately when the previous step has
already taken that long. Supply a positive finite frequency in hertz.
Timing changes wake the worker. A step already in progress completes with
its original execution mode. The elapsed time supplied to the first step
after start or resume measures time since that transition.
