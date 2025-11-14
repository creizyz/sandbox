#include "module.h"

IModule::IModule()
    : m_state(State::CREATED)
{ };

bool IModule::init()
{
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
    switch (m_state)
    {
        case State::INITIALIZED:
        {
            if (onStart())
            {
                m_state = State::RUNNING;
                return true;
            }
            return false;
        }
        case State::PAUSED:
        {
            if (onResume())
            {
                m_state = State::RUNNING;
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
    switch (m_state)
    {
        case State::RUNNING:
        case State::PAUSED:
        {
            if (onStop())
            {
                m_state = State::STOPPED;
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


bool IModule::onInit()
{
    // do nothing
    return true;
};
bool IModule::onRelease()
{
    // do nothing
    return true;
};
bool IModule::onStart()
{
    // do nothing
    return true;
};
bool IModule::onStop()
{
    // do nothing
    return true;
};
bool IModule::onPause()
{
    // do nothing
    return true;
};
bool IModule::onResume()
{
    // do nothing
    return true;
}

bool IModule::onShutdown()
{
    // do nothing
    return true;
};

bool IModule::onReset()
{
    // do nothing
    return true;
};