#include "logger.hpp"

log::Logger::Logger(std::string name)
{
}

const std::string& log::Logger::name() const noexcept
{
}

void log::Logger::set_level(Level level) noexcept
{
}

log::Level log::Logger::level() const noexcept
{
}

void log::Logger::add_sink(std::shared_ptr<Sink> sink)
{
}

bool log::Logger::enabled(Level level) const noexcept
{
}

void log::Logger::log(Level level, std::string_view message, std::source_location location)
{
}

void log::Logger::trace(std::string_view message, std::source_location location)
{
}

void log::Logger::debug(std::string_view message, std::source_location location)
{
}

void log::Logger::info(std::string_view message, std::source_location location)
{
}

void log::Logger::warning(std::string_view message, std::source_location location)
{
}

void log::Logger::error(std::string_view message, std::source_location location)
{
}

void log::Logger::critical(std::string_view message, std::source_location location)
{
}

void log::Logger::flush()
{
}

void log::ConsoleSink::write(const Record& record)
{
}

void log::ConsoleSink::flush()
{
}

log::FileSink::FileSink(std::string path)
{
}

void log::FileSink::write(const Record& record)
{
}

void log::FileSink::flush()
{
}

log::Logger& log::default_logger()
{
}
