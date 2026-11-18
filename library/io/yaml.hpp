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
    struct YamlLocation
    {
        std::size_t m_line;
        std::size_t m_column;
    };

    struct YamlError
    {
        std::string m_message;
        std::optional<YamlLocation> m_location;
    };

    // Read one YAML document using the portable, string-keyed value model.
    [[nodiscard]] std::expected<DocumentValue, YamlError> parse_yaml(std::string_view input);
    [[nodiscard]] std::expected<DocumentValue, YamlError> read_yaml(std::istream &input);

    [[nodiscard]] std::expected<std::string, YamlError> write_yaml(const DocumentValue &value);
    [[nodiscard]] std::expected<void, YamlError> write_yaml(std::ostream &output, const DocumentValue &value);
}
