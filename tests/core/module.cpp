#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <utility>

#include "core/module.hpp"

namespace
{
    struct Events
    {
        std::mutex m_mutex;
        std::condition_variable m_cv;
        unsigned m_steps = 0;
        unsigned m_pauses = 0;
        unsigned m_resumes = 0;
        unsigned m_stops = 0;
        unsigned m_releases = 0;
        bool m_destroyed = false;

        bool wait_for_steps(unsigned count)
        {
            std::unique_lock lock(m_mutex);
            return m_cv.wait_for(lock, std::chrono::seconds(5), [&]
                                 { return m_steps >= count; });
        }
    };

    class CountingModule final : public IModule
    {
    public:
        explicit CountingModule(Events &events)
            : m_events{events} {}
        ~CountingModule() override
        {
            m_events.m_destroyed = true;
        }

    private:
        Events &m_events;

        void step(std::chrono::duration<double>) noexcept override
        {
            std::lock_guard lock(m_events.m_mutex);
            ++m_events.m_steps;
            m_events.m_cv.notify_all();
        }
        bool on_pause() noexcept override
        {
            ++m_events.m_pauses;
            return true;
        }
        bool on_resume() noexcept override
        {
            ++m_events.m_resumes;
            return true;
        }
        bool on_stop() noexcept override
        {
            ++m_events.m_stops;
            return true;
        }
        bool on_release() noexcept override
        {
            ++m_events.m_releases;
            return true;
        }
    };

    TEST(ModuleRunner, RestartsCompletedOneShotAfterResetOrRelease)
    {
        Events events;
        ModuleRunner runner(std::make_unique<CountingModule>(events));
        ASSERT_TRUE(runner.init());
        ASSERT_TRUE(runner.start());
        ASSERT_TRUE(events.wait_for_steps(1));
        ASSERT_TRUE(runner.stop());
        EXPECT_FALSE(runner.is_running());
        ASSERT_TRUE(runner.reset());
        ASSERT_TRUE(runner.start());
        ASSERT_TRUE(events.wait_for_steps(2));
        ASSERT_TRUE(runner.stop());
        ASSERT_TRUE(runner.release());
        ASSERT_TRUE(runner.init());
        ASSERT_TRUE(runner.start());
        ASSERT_TRUE(events.wait_for_steps(3));
        ASSERT_TRUE(runner.stop());
        EXPECT_EQ(events.m_steps, 3u);
    }

    TEST(ModuleRunner, PausesResumesAndInterruptsFixedRateWaits)
    {
        Events events;
        {
            ModuleRunner runner(std::make_unique<CountingModule>(events), ModuleRunner::ExecutionMode::FIXED_RATE, 1.0 / 3600.0);
            ASSERT_TRUE(runner.init());
            ASSERT_TRUE(runner.start());
            ASSERT_TRUE(events.wait_for_steps(1));
            ASSERT_TRUE(runner.pause());
            EXPECT_FALSE(runner.is_running());
            ASSERT_TRUE(runner.pause());
            ASSERT_TRUE(runner.start());
            ASSERT_TRUE(events.wait_for_steps(2));
            runner.set_execution_mode(ModuleRunner::ExecutionMode::MAX_RATE);
            ASSERT_TRUE(events.wait_for_steps(3));
        }
        EXPECT_EQ(events.m_pauses, 1u);
        EXPECT_EQ(events.m_resumes, 1u);
        EXPECT_EQ(events.m_stops, 1u);
        EXPECT_EQ(events.m_releases, 1u);
        EXPECT_TRUE(events.m_destroyed);
    }

    class CleanupModule final : public IModule
    {
    public:
        ModuleRunner *m_runner = nullptr;
        bool m_first_start = true;
        bool &m_released;

        explicit CleanupModule(bool &released)
            : m_released(released) {}
        void step(std::chrono::duration<double>) override {}
        bool on_start() override
        {
            if (m_first_start)
            {
                m_first_start = false;
                throw std::runtime_error("start failed");
            }
            return true;
        }
        bool on_stop() override
        {
            throw std::runtime_error("stop failed");
        }
        bool on_release() override
        {
            m_released = !m_runner->is_running();
            return true;
        }
    };

    TEST(ModuleRunner, RetriesAfterAHookThrowsAndFinishesCleanup)
    {
        bool released = false;
        {
            auto module = std::make_unique<CleanupModule>(released);
            auto &hooks = *module;
            ModuleRunner runner(std::move(module), ModuleRunner::ExecutionMode::FIXED_RATE, 1.0);
            hooks.m_runner = &runner;
            ASSERT_TRUE(runner.init());
            EXPECT_THROW(runner.start(), std::runtime_error);
            EXPECT_FALSE(runner.is_running());
            ASSERT_TRUE(runner.start());
            EXPECT_THROW(runner.stop(), std::runtime_error);
            EXPECT_TRUE(runner.is_running());
        }
        EXPECT_TRUE(released);
    }

    struct StepGate
    {
        std::binary_semaphore m_entered{0};
        std::binary_semaphore m_finish{0};
        std::atomic<bool> m_finished = false;
        std::atomic<bool> m_stopped_after_step = false;
    };

    class BlockingModule final : public IModule
    {
    public:
        explicit BlockingModule(StepGate &gate)
            : m_gate(gate) {}

    private:
        StepGate &m_gate;
        void step(std::chrono::duration<double>) override
        {
            m_gate.m_entered.release();
            m_gate.m_finish.acquire();
            m_gate.m_finished = true;
        }
        bool on_stop() override
        {
            m_gate.m_stopped_after_step = m_gate.m_finished.load();
            return true;
        }
    };

    TEST(ModuleRunner, StopAndDestructionWaitForAnActiveStep)
    {
        for (bool destroy : {false, true})
        {
            StepGate gate;
            auto runner = std::make_unique<ModuleRunner>(std::make_unique<BlockingModule>(gate), ModuleRunner::ExecutionMode::FIXED_RATE, 1.0 / 3600.0);
            ASSERT_TRUE(runner->init());
            ASSERT_TRUE(runner->start());
            const bool entered = gate.m_entered.try_acquire_for(std::chrono::seconds(5));
            if (!entered)
            {
                gate.m_finish.release();
                FAIL() << "Worker did not enter step";
            }
            std::binary_semaphore stopping{0};
            std::atomic<bool> returned = false;
            std::jthread control([&]
                                 {
                stopping.release();
                if (destroy)
                {
                    runner.reset();
                }
                else
                {
                    EXPECT_TRUE(runner->stop());
                }
                returned = true; });
            stopping.acquire();
            EXPECT_FALSE(returned.load());
            gate.m_finish.release();
            control.join();
            EXPECT_TRUE(gate.m_stopped_after_step.load());
        }
    }
}
