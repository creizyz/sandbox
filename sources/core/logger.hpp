#pragma once

#include <memory>
#include <source_location>
#include <string>
#include <string_view>

namespace log {

enum class Level {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
    off
};

struct Record {
    Level level;
    std::string message;
    std::source_location location;
};

class Sink {
public:
    virtual ~Sink() = default;

    virtual void write(const Record& record) = 0;
    virtual void flush() = 0;
};

class Logger {
public:
    explicit Logger(std::string name);

    const std::string& name() const noexcept;

    void set_level(Level level) noexcept;
    Level level() const noexcept;

    void add_sink(std::shared_ptr<Sink> sink);

    bool enabled(Level level) const noexcept;

    void log(
        Level level,
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void trace(
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void debug(
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void info(
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void warning(
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void error(
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void critical(
        std::string_view message,
        std::source_location location = std::source_location::current()
    );

    void flush();
};

class ConsoleSink : public Sink {
public:
    ~ConsoleSink() override = default;

    void write(const Record& record) override;
    void flush() override;
};

class FileSink : public Sink {
public:
    explicit FileSink(std::string path);

    ~FileSink() override = default;

    void write(const Record& record) override;
    void flush() override;
};

Logger& default_logger();

} // namespace log