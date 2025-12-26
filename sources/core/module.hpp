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
    double m_frequencyHz; // when running in fixed rate mode

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit IModule(ExecutionMode mode = ExecutionMode::ONCE, double frequencyHz = 0.0);
    virtual ~IModule();

    bool init();
    bool release();
    bool start();
    bool stop();
    bool pause();
    bool reset();

    [[nodiscard]] bool isRunning() const { return m_state == State::RUNNING; }

    void setExecutionMode(ExecutionMode mode, double frequencyHz = 0.0);

protected:
    virtual bool onInit();
    virtual bool onRelease();
    virtual bool onStart();
    virtual bool onStop();
    virtual bool onPause();
    virtual bool onResume();
    virtual bool onReset();

    virtual void step(std::chrono::duration<double> dT) = 0;

private:
    void run();
};