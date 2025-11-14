#pragma once

class IModule {
private:
    enum class State {
        CREATED,
        INITIALIZED,
        RUNNING,
        PAUSED,
        STOPPED
    };

    State m_state;

public:
    IModule();

    virtual ~IModule() = default;

    bool init();
    bool release();
    bool start();
    bool stop();
    bool pause();
    bool reset();

    virtual bool onInit();
    virtual bool onRelease();
    virtual bool onStart();
    virtual bool onStop();
    virtual bool onPause();
    virtual bool onResume();
    virtual bool onShutdown();
    virtual bool onReset();

    virtual void shutdown() = 0;
    virtual void executionLoop() = 0;
};