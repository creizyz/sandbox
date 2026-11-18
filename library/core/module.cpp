#include "module.hpp"

IModule::IModule(ExecutionMode mode, double frequency_hz)
    : m_state{ State::CREATED }
    , m_mode{ mode }
    , m_frequency_hz{ frequency_hz }
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
            if (on_init())
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
            if (on_release())
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
            if (on_start())
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
            if (on_resume())
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
            if (on_stop())
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
            if (on_pause())
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
            if (on_reset())
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

void IModule::set_execution_mode(ExecutionMode mode, double frequency_hz)
{
    m_mode = mode;
    m_frequency_hz = frequency_hz;
}

bool IModule::on_init()
{
    // do nothing
    return true;
}

bool IModule::on_release()
{
    // do nothing
    return true;
}

bool IModule::on_start()
{
    // do nothing
    return true;
}

bool IModule::on_stop()
{
    // do nothing
    return true;
}

bool IModule::on_pause()
{
    // do nothing
    return true;
}

bool IModule::on_resume()
{
    // do nothing
    return true;
}

bool IModule::on_reset()
{
    // do nothing
    return true;
}

void IModule::run()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto get_period = [this]() -> std::chrono::duration<double>
    {
        return (m_mode == ExecutionMode::FIXED_RATE && m_frequency_hz > 0.0)
               ? std::chrono::duration<double>(1.0 / m_frequency_hz)
               : std::chrono::duration<double>::zero();
    };

    auto previous_iteration_timestamp = std::chrono::steady_clock::now();

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
        auto dt = std::chrono::duration<double>(now - previous_iteration_timestamp);
        previous_iteration_timestamp = now;

        auto mode = m_mode;
        auto period = get_period();
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
            auto start_timestamp = std::chrono::steady_clock::now();
            step(dt);
            auto current_step_duration = std::chrono::steady_clock::now() - start_timestamp;


            lock.lock();
            if (m_state != State::RUNNING) // If paused or stopped, loop up and wait/exit
            {
                continue;
            }

            auto sleep_time = period - current_step_duration;
            if (sleep_time > std::chrono::duration<double>::zero())
            {
                // Sleep but allow interruption by pause/stop or mode change
                m_cv.wait_for(lock, sleep_time, [this] {
                    return m_state != State::RUNNING;
                });
            }
        }
    }
}

