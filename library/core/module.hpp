#pragma once

#include <chrono>
#include <condition_variable>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <thread>

class ModuleRunner;

// Implement the work and optional lifecycle hooks. ModuleRunner owns their lifetime.
class IModule
{
public:
    enum class ExecutionMode
    {
        ONCE,
        FIXED_RATE,
        MAX_RATE
    };
    virtual ~IModule() = default;

protected:
    friend class ModuleRunner;
    virtual bool on_init()
    {
        return true;
    }
    virtual bool on_release()
    {
        return true;
    }
    virtual bool on_start()
    {
        return true;
    }
    virtual bool on_stop()
    {
        return true;
    }
    virtual bool on_pause()
    {
        return true;
    }
    virtual bool on_resume()
    {
        return true;
    }
    virtual bool on_reset()
    {
        return true;
    }
    virtual void step(std::chrono::duration<double> dt) = 0;
};

// Lifecycle calls are synchronous and serialize with step(). Hooks may query the runner
// or change its timing. Call lifecycle methods and destroy the runner from outside its hooks.
class ModuleRunner final
{
public:
    using ExecutionMode = IModule::ExecutionMode;
    explicit ModuleRunner(std::unique_ptr<IModule> module, ExecutionMode mode = ExecutionMode::ONCE, double frequency_hz = 0.0);
    ~ModuleRunner();

    bool init();
    bool release();
    bool start();
    bool stop();
    bool pause();
    bool reset();
    [[nodiscard]] bool is_running() const;
    // FIXED_RATE requires a positive, finite frequency; invalid settings throw std::invalid_argument.
    void set_execution_mode(ExecutionMode mode, double frequency_hz = 0.0);

private:
    enum class State
    {
        CREATED,
        INITIALIZED,
        RUNNING,
        PAUSED,
        STOPPED
    };
    struct Transition
    {
        State m_source;
        bool (IModule::*m_hook)();
    };
    bool change(State target, std::initializer_list<Transition> transitions);
    void run();

    std::unique_ptr<IModule> m_module;
    std::mutex m_control;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
    State m_state = State::CREATED;
    ExecutionMode m_mode = ExecutionMode::ONCE;
    double m_frequency_hz = 0.0;
    std::chrono::steady_clock::time_point m_last_step{};
    bool m_control_pending = false;
    bool m_stepping = false;
    bool m_schedule_changed = false;
    bool m_shutdown = false;
};
