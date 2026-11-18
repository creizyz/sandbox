#pragma once

#include <cstddef>
#include <expected>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

#include "document-value.hpp"

namespace io
{
    using JsonNumber = Number;
    using JsonValue = DocumentValue;

    struct JsonError
    {
        std::string_view m_message;
        std::optional<std::size_t> m_offset;
    };

    // Parse one complete document. Comments are allowed between tokens.
    [[nodiscard]] std::expected<DocumentValue, JsonError> parse_json(std::string_view input);
    [[nodiscard]] std::expected<DocumentValue, JsonError> read_json(std::istream &input);

    // Write standard JSON. Pretty output uses two spaces per level.
    [[nodiscard]] std::expected<std::string, JsonError> write_json(const DocumentValue &value, bool pretty = false);
    [[nodiscard]] std::expected<void, JsonError> write_json(std::ostream &output, const DocumentValue &value, bool pretty = false);
}
