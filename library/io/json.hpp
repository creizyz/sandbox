#pragma once

#include <cstddef>
#include <expected>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace io
{
    struct JsonNumber
    {
        // The JSON token is retained exactly, including precision and exponent spelling.
        std::string m_text;

        bool operator==(const JsonNumber &) const = default;
    };

    struct JsonValue
    {
        using Array = std::vector<JsonValue>;
        using Object = std::map<std::string, JsonValue>;
        using Data = std::variant<std::nullptr_t, bool, JsonNumber, std::string, Array, Object>;

        Data m_data{nullptr};

        bool operator==(const JsonValue &) const = default;
    };

    struct JsonError
    {
        std::string_view m_message;
        std::optional<std::size_t> m_offset;
    };

    // Parse one complete document. Comments are allowed between tokens.
    [[nodiscard]] std::expected<JsonValue, JsonError> parse_json(std::string_view input);
    [[nodiscard]] std::expected<JsonValue, JsonError> read_json(std::istream &input);

    // Write standard JSON. Pretty output uses two spaces per level.
    [[nodiscard]] std::expected<std::string, JsonError> write_json(const JsonValue &value, bool pretty = false);
    [[nodiscard]] std::expected<void, JsonError> write_json(std::ostream &output, const JsonValue &value, bool pretty = false);
}
