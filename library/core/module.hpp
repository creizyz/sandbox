#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

class IModule {
public:
    enum class ExecutionMode {
        ONCE,
        FIXED_RATE,
        MAX_RATE
    };

private:
    enum class State {
        CREATED,
        INITIALIZED,
        RUNNING,
        PAUSED,
        STOPPED
    };

    State m_state;
    ExecutionMode m_mode;
    double m_frequency_hz; // when running in fixed rate mode

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit IModule(ExecutionMode mode = ExecutionMode::ONCE, double frequency_hz = 0.0);
    virtual ~IModule();

    bool init();
    bool release();
    bool start();
    bool stop();
    bool pause();
    bool reset();

    [[nodiscard]] bool is_running() const { return m_state == State::RUNNING; }

    void set_execution_mode(ExecutionMode mode, double frequency_hz = 0.0);

protected:
    virtual bool on_init();
    virtual bool on_release();
    virtual bool on_start();
    virtual bool on_stop();
    virtual bool on_pause();
    virtual bool on_resume();
    virtual bool on_reset();

    virtual void step(std::chrono::duration<double> dt) = 0;

private:
    void run();
};