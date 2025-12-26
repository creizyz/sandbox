#include "module.hpp"

IModule::IModule(ExecutionMode mode, double frequencyHz)
    : m_state{ State::CREATED }
    , m_mode{ mode }
    , m_frequencyHz{ frequencyHz }
{ }

IModule::~IModule()
{
    this->stop();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    this->release();
}

bool IModule::init()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (m_state)
    {
        case State::CREATED:
        {
            if (onInit())
            {
                m_state = State::INITIALIZED;
                return true;
            }
            return false;
        }
        case State::INITIALIZED:
        {
            // do nothing
            return true;
        }
        default:
        {
            // TODO: log warning
            return false;
        }
    }
}

bool IModule::release()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (m_state)
    {
        case State::INITIALIZED:
        case State::STOPPED:
        {
            if (onRelease())
            {
                m_state = State::CREATED;
                return true;
            }
            return false;
        }
        case State::CREATED:
        {
            // do nothing
            return true;
        }
        default:
        {
            // TODO: log warning
            return false;
        }
    }
}

bool IModule::start()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    switch (m_state)
    {
        case State::INITIALIZED:
        {
            if (onStart())
            {
                m_state = State::RUNNING;
                if (!m_thread.joinable())
                {
                    m_thread = std::thread(&IModule::run, this);
                }
                lock.unlock();
                m_cv.notify_all();
                return true;
            }
            return false;
        }
        case State::PAUSED:
        {
            if (onResume())
            {
                m_state = State::RUNNING;
                lock.unlock();
                m_cv.notify_all();
                return true;
            }
            return false;
        }
        case State::RUNNING:
        {
            // do nothing
            return true;
        }
        default:
        {
            // TODO: log warning
            return false;
        }
    }
}

bool IModule::stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    switch (m_state)
    {
        case State::RUNNING:
        case State::PAUSED:
        {
            if (onStop())
            {
                m_state = State::STOPPED;
                lock.unlock();
                m_cv.notify_all();
                return true;
            }
            return false;
        }
        case State::STOPPED:
        {
            // do nothing
            return true;
        }
        default:
        {
            // TODO: log warning
            return false;
        }
    }
}

bool IModule::pause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (m_state)
    {
        case State::RUNNING:
        {
            if (onPause())
            {
                m_state = State::STOPPED;
                return true;
            }
            return false;
        }
        case State::PAUSED:
        {
            // do nothing
            return true;
        }
        default:
        {
            // TODO: log warning
            return false;
        }
    }
}

bool IModule::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (m_state)
    {
        case State::STOPPED:
        {
            if (onReset())
            {
                m_state = State::INITIALIZED;
                return true;
            }
            return false;
        }
        case State::INITIALIZED:
        {
            // do nothing
            return true;
        }
        default:
        {
            // TODO: log warning
            return false;
        }
    }
}

void IModule::setExecutionMode(ExecutionMode mode, double frequencyHz)
{
    m_mode = mode;
    m_frequencyHz = frequencyHz;
}

bool IModule::onInit()
{
    // do nothing
    return true;
}

bool IModule::onRelease()
{
    // do nothing
    return true;
}

bool IModule::onStart()
{
    // do nothing
    return true;
}

bool IModule::onStop()
{
    // do nothing
    return true;
}

bool IModule::onPause()
{
    // do nothing
    return true;
}

bool IModule::onResume()
{
    // do nothing
    return true;
}

bool IModule::onReset()
{
    // do nothing
    return true;
}

void IModule::run()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto getPeriod = [this]() -> std::chrono::duration<double>
    {
        return (m_mode == ExecutionMode::FIXED_RATE && m_frequencyHz > 0.0)
               ? std::chrono::duration<double>(1.0 / m_frequencyHz)
               : std::chrono::duration<double>::zero();
    };

    auto previousIterationTimestamp = std::chrono::steady_clock::now();

    while (true)
    {
        // Wait until RUNNING or STOPPED
        m_cv.wait(lock, [this] {
            return m_state == State::RUNNING || m_state == State::STOPPED;
        });

        if (m_state == State::STOPPED)
        {
            // TODO add exit log
            break;
        }

        auto now = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration<double>(now - previousIterationTimestamp);
        previousIterationTimestamp = now;

        auto mode = m_mode;
        auto period = getPeriod();
        lock.unlock();

        if (mode == ExecutionMode::ONCE)
        {
            step(dt);
            lock.lock();
            m_state = State::STOPPED;
            break;
        }

        if (mode == ExecutionMode::MAX_RATE)
        {
            step(dt);
            lock.lock();
            continue;
        }

        if (mode == ExecutionMode::FIXED_RATE)
        {
            // execute current step
            auto startTimestamp = std::chrono::steady_clock::now();
            step(dt);
            auto currentStepDuration = std::chrono::steady_clock::now() - startTimestamp;


            lock.lock();
            if (m_state != State::RUNNING) // If paused or stopped, loop up and wait/exit
            {
                continue;
            }

            auto sleepTime = period - currentStepDuration;
            if (sleepTime > std::chrono::duration<double>::zero())
            {
                // Sleep but allow interruption by pause/stop or mode change
                m_cv.wait_for(lock, sleepTime, [this] {
                    return m_state != State::RUNNING;
                });
            }
        }
    }
}

