#include <gtest/gtest.h>

#include "core/logger.hpp"

#include <memory>
#include <string>
#include <vector>

namespace {

class TestSink final : public log::Sink {
public:
    void write(const log::Record& record) override {
        records.push_back(record);
    }

    void flush() override {
        ++flush_count;
    }

    std::vector<log::Record> records;
    int flush_count = 0;
};

} // namespace

TEST(LoggerApiTest, StoresLoggerName) {
    log::Logger logger{"app"};

    EXPECT_EQ(logger.name(), "app");
}

TEST(LoggerApiTest, StoresLogLevel) {
    log::Logger logger{"app"};

    logger.set_level(log::Level::warning);

    EXPECT_EQ(logger.level(), log::Level::warning);
}

TEST(LoggerApiTest, EnabledReflectsConfiguredLevel) {
    log::Logger logger{"app"};

    logger.set_level(log::Level::warning);

    EXPECT_FALSE(logger.enabled(log::Level::debug));
    EXPECT_FALSE(logger.enabled(log::Level::info));
    EXPECT_TRUE(logger.enabled(log::Level::warning));
    EXPECT_TRUE(logger.enabled(log::Level::error));
    EXPECT_TRUE(logger.enabled(log::Level::critical));
}

TEST(LoggerApiTest, OffDisablesAllLogging) {
    log::Logger logger{"app"};

    logger.set_level(log::Level::off);

    EXPECT_FALSE(logger.enabled(log::Level::trace));
    EXPECT_FALSE(logger.enabled(log::Level::debug));
    EXPECT_FALSE(logger.enabled(log::Level::info));
    EXPECT_FALSE(logger.enabled(log::Level::warning));
    EXPECT_FALSE(logger.enabled(log::Level::error));
    EXPECT_FALSE(logger.enabled(log::Level::critical));
}

TEST(LoggerApiTest, LogWritesToSinkWhenEnabled) {
    log::Logger logger{"app"};
    auto sink = std::make_shared<TestSink>();

    logger.set_level(log::Level::info);
    logger.add_sink(sink);

    logger.log(log::Level::info, "hello");

    ASSERT_EQ(sink->records.size(), 1u);
    EXPECT_EQ(sink->records[0].level, log::Level::info);
    EXPECT_EQ(sink->records[0].message, "hello");
}

TEST(LoggerApiTest, LogDoesNotWriteToSinkWhenDisabled) {
    log::Logger logger{"app"};
    auto sink = std::make_shared<TestSink>();

    logger.set_level(log::Level::error);
    logger.add_sink(sink);

    logger.log(log::Level::debug, "hidden");

    EXPECT_TRUE(sink->records.empty());
}

TEST(LoggerApiTest, WritesToMultipleSinks) {
    log::Logger logger{"app"};
    auto first_sink = std::make_shared<TestSink>();
    auto second_sink = std::make_shared<TestSink>();

    logger.set_level(log::Level::trace);
    logger.add_sink(first_sink);
    logger.add_sink(second_sink);

    logger.info("message");

    ASSERT_EQ(first_sink->records.size(), 1u);
    ASSERT_EQ(second_sink->records.size(), 1u);

    EXPECT_EQ(first_sink->records[0].message, "message");
    EXPECT_EQ(second_sink->records[0].message, "message");
}

TEST(LoggerApiTest, ConvenienceMethodsUseCorrectLevels) {
    log::Logger logger{"app"};
    auto sink = std::make_shared<TestSink>();

    logger.set_level(log::Level::trace);
    logger.add_sink(sink);

    logger.trace("trace");
    logger.debug("debug");
    logger.info("info");
    logger.warning("warning");
    logger.error("error");
    logger.critical("critical");

    ASSERT_EQ(sink->records.size(), 6u);

    EXPECT_EQ(sink->records[0].level, log::Level::trace);
    EXPECT_EQ(sink->records[1].level, log::Level::debug);
    EXPECT_EQ(sink->records[2].level, log::Level::info);
    EXPECT_EQ(sink->records[3].level, log::Level::warning);
    EXPECT_EQ(sink->records[4].level, log::Level::error);
    EXPECT_EQ(sink->records[5].level, log::Level::critical);
}

TEST(LoggerApiTest, FlushFlushesAllSinks) {
    log::Logger logger{"app"};
    auto first_sink = std::make_shared<TestSink>();
    auto second_sink = std::make_shared<TestSink>();

    logger.add_sink(first_sink);
    logger.add_sink(second_sink);

    logger.flush();

    EXPECT_EQ(first_sink->flush_count, 1);
    EXPECT_EQ(second_sink->flush_count, 1);
}

TEST(LoggerApiTest, CapturesSourceLocation) {
    log::Logger logger{"app"};
    auto sink = std::make_shared<TestSink>();

    logger.set_level(log::Level::info);
    logger.add_sink(sink);

    const auto expected_line = __LINE__ + 1;
    logger.info("with location");

    ASSERT_EQ(sink->records.size(), 1u);
    EXPECT_EQ(sink->records[0].location.line(), expected_line);
}

TEST(LoggerApiTest, DefaultLoggerIsAccessible) {
    log::Logger& logger = log::default_logger();

    EXPECT_NO_THROW(logger.info("default logger message"));
}