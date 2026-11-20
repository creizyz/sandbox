#include "module.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

ModuleRunner::ModuleRunner(std::unique_ptr<IModule> module, ExecutionMode mode, double frequency_hz)
    : m_module{std::move(module)}
{
    if (!m_module)
    {
        throw std::invalid_argument("module is required");
    }
    set_execution_mode(mode, frequency_hz);
}

ModuleRunner::~ModuleRunner()
{
    {
        std::lock_guard lock(m_mutex);
        m_shutdown = true;
    }
    m_cv.notify_all();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    // Explicit stop/release report hook failures. Destruction must finish both cleanup stages.
    const auto cleanup = [this](bool (IModule::*hook)())
    {
        try
        {
            ((*m_module).*hook)();
        }
        catch (...)
        {
            // Keep dismantling the module even when user cleanup throws.
        }
    };
    if (m_state == State::RUNNING || m_state == State::PAUSED)
    {
        cleanup(&IModule::on_stop);
        m_state = State::STOPPED;
    }
    if (m_state != State::CREATED)
    {
        cleanup(&IModule::on_release);
    }
}

bool ModuleRunner::init()
{
    return change(State::INITIALIZED, {{State::CREATED, &IModule::on_init}});
}

bool ModuleRunner::release()
{
    return change(State::CREATED, {{State::INITIALIZED, &IModule::on_release}, {State::STOPPED, &IModule::on_release}});
}

bool ModuleRunner::start()
{
    return change(State::RUNNING, {{State::INITIALIZED, &IModule::on_start}, {State::PAUSED, &IModule::on_resume}});
}

bool ModuleRunner::stop()
{
    return change(State::STOPPED, {{State::RUNNING, &IModule::on_stop}, {State::PAUSED, &IModule::on_stop}});
}

bool ModuleRunner::pause()
{
    return change(State::PAUSED, {{State::RUNNING, &IModule::on_pause}});
}

bool ModuleRunner::reset()
{
    return change(State::INITIALIZED, {{State::STOPPED, &IModule::on_reset}});
}

bool ModuleRunner::is_running() const
{
    std::lock_guard lock(m_mutex);
    return m_state == State::RUNNING;
}

bool ModuleRunner::change(State target, std::initializer_list<Transition> transitions)
{
    std::lock_guard control(m_control);
    std::unique_lock lock(m_mutex);
    if (m_state == target)
    {
        return true;
    }
    if (m_state == State::INITIALIZED && target == State::RUNNING && !m_thread.joinable())
    {
        m_thread = std::thread(&ModuleRunner::run, this);
    }
    // Stop admitting work before waiting for the current step to finish.
    m_control_pending = true;
    m_cv.notify_all();
    m_cv.wait(lock, [this]
              { return !m_stepping; });
    bool success = m_state == target; // An ONCE step may have just completed.
    const auto action = std::find_if(transitions.begin(), transitions.end(), [this](const auto &entry)
                                     { return entry.m_source == m_state; });
    if (action != transitions.end())
    {
        lock.unlock();
        try
        {
            success = ((*m_module).*(action->m_hook))();
        }
        catch (...)
        {
            lock.lock();
            m_control_pending = false;
            lock.unlock();
            m_cv.notify_all();
            throw;
        }
        lock.lock();
        if (success)
        {
            m_state = target;
            m_schedule_changed = true;
            if (target == State::RUNNING)
            {
                m_last_step = std::chrono::steady_clock::now();
            }
        }
    }
    m_control_pending = false;
    lock.unlock();
    m_cv.notify_all();
    return success;
}

void ModuleRunner::set_execution_mode(ExecutionMode mode, double frequency_hz)
{
    if (mode == ExecutionMode::FIXED_RATE && (!std::isfinite(frequency_hz) || frequency_hz <= 0.0))
    {
        throw std::invalid_argument("fixed rate requires a positive, finite frequency");
    }
    {
        std::lock_guard lock(m_mutex);
        if (m_mode == mode && m_frequency_hz == frequency_hz)
        {
            return;
        }
        m_schedule_changed = true;
        m_mode = mode;
        m_frequency_hz = frequency_hz;
    }
    m_cv.notify_all();
}

void ModuleRunner::run()
{
    using Clock = std::chrono::steady_clock;
    std::unique_lock lock(m_mutex);
    while (true)
    {
        m_cv.wait(lock, [this]
                  { return m_shutdown || (m_state == State::RUNNING && !m_control_pending); });
        if (m_shutdown)
        {
            return;
        }
        const auto mode = m_mode;
        const auto frequency = m_frequency_hz;
        const auto now = Clock::now();
        const auto dt = now - m_last_step;
        m_last_step = now;
        m_schedule_changed = false;
        m_stepping = true;
        lock.unlock();
        m_module->step(dt);
        lock.lock();
        m_stepping = false;
        if (m_control_pending)
        {
            m_cv.notify_all();
        }
        if (mode == ExecutionMode::ONCE)
        {
            m_state = State::STOPPED;
        }
        else if (mode == ExecutionMode::FIXED_RATE)
        {
            const std::chrono::duration<double> period(1.0 / frequency);
            const auto deadline = period >= Clock::time_point::max() - now ? Clock::time_point::max() : now + std::chrono::duration_cast<Clock::duration>(period);
            // Remember even a pause/resume pair that completes before the worker wakes.
            m_cv.wait_until(lock, deadline, [this]
                            { return m_shutdown || m_schedule_changed; });
        }
    }
}
