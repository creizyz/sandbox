#include "json.hpp"

#include "detail/number.hpp"
#include "detail/utf8.hpp"

#include <cstdint>
#include <istream>
#include <iterator>
#include <ostream>
#include <type_traits>
#include <utility>

namespace io
{
    namespace
    {
        constexpr std::size_t max_depth = 256;
        constexpr std::string_view hex_digits = "0123456789abcdef";

        void append_utf8(std::string &output, std::uint32_t code_point)
        {
            if (code_point < 0x80)
            {
                output += static_cast<char>(code_point);
                return;
            }

            const auto append_continuation = [&](std::uint32_t shift)
            {
                output += static_cast<char>(0x80 | ((code_point >> shift) & 0x3f));
            };
            if (code_point >= 0x10000)
            {
                output += static_cast<char>(0xf0 | (code_point >> 18));
                append_continuation(12);
                append_continuation(6);
            }
            else if (code_point >= 0x800)
            {
                output += static_cast<char>(0xe0 | (code_point >> 12));
                append_continuation(6);
            }
            else
            {
                output += static_cast<char>(0xc0 | (code_point >> 6));
            }
            append_continuation(0);
        }

        class Parser
        {
        public:
            explicit Parser(std::string_view input)
                : m_input{input}
            {
            }

            std::expected<DocumentValue, JsonError> parse()
            {
                auto value = parse_value(0);
                if (!value)
                {
                    return value;
                }
                if (auto failure = skip_trivia())
                {
                    return std::unexpected(*failure);
                }
                if (m_pos != m_input.size())
                {
                    return error("unexpected trailing input");
                }
                return value;
            }

        private:
            std::string_view m_input;
            std::size_t m_pos = 0;

            char peek() const
            {
                return m_pos < m_input.size() ? m_input[m_pos] : '\0';
            }

            bool consume(std::string_view token)
            {
                if (!m_input.substr(m_pos).starts_with(token))
                {
                    return false;
                }
                m_pos += token.size();
                return true;
            }

            std::unexpected<JsonError> error(std::string_view message) const
            {
                return std::unexpected(JsonError{message, m_pos});
            }

            std::optional<JsonError> skip_trivia()
            {
                while (m_pos < m_input.size())
                {
                    switch (peek())
                    {
                    case ' ':
                    case '\t':
                    case '\r':
                    case '\n':
                        ++m_pos;
                        continue;
                    default:
                        break;
                    }
                    const auto start = m_pos;
                    if (consume("//"))
                    {
                        const auto end = m_input.find_first_of("\r\n", m_pos);
                        m_pos = end == std::string_view::npos ? m_input.size() : end;
                    }
                    else if (consume("/*"))
                    {
                        const auto end = m_input.find("*/", m_pos);
                        if (end == std::string_view::npos)
                        {
                            return JsonError{"unterminated block comment", start};
                        }
                        m_pos = end + 2;
                    }
                    else
                    {
                        break;
                    }
                }
                return std::nullopt;
            }

            std::expected<DocumentValue, JsonError> parse_value(std::size_t depth)
            {
                if (auto failure = skip_trivia())
                {
                    return std::unexpected(*failure);
                }
                const auto first = peek();
                if (first == '{' || first == '[')
                {
                    if (depth == max_depth)
                    {
                        return error("nesting limit exceeded");
                    }
                    return first == '{' ? parse_object(depth + 1) : parse_array(depth + 1);
                }
                if (first == '"')
                {
                    return parse_string().transform([](std::string &&text)
                                                    { return DocumentValue{std::move(text)}; });
                }
                if (consume("null"))
                {
                    return DocumentValue{};
                }
                if (consume("true"))
                {
                    return DocumentValue{true};
                }
                if (consume("false"))
                {
                    return DocumentValue{false};
                }
                if (first == '-' || detail::is_digit(first))
                {
                    const auto start = m_pos;
                    if (!detail::scan_number(m_input, m_pos))
                    {
                        return error("invalid number");
                    }
                    return DocumentValue{Number{std::string(m_input.substr(start, m_pos - start))}};
                }
                return error("expected a JSON value");
            }

            std::expected<DocumentValue, JsonError> parse_array(std::size_t depth)
            {
                ++m_pos;
                DocumentValue::Array array;
                if (auto failure = skip_trivia())
                {
                    return std::unexpected(*failure);
                }
                if (consume("]"))
                {
                    return DocumentValue{std::move(array)};
                }
                while (true)
                {
                    auto value = parse_value(depth);
                    if (!value)
                    {
                        return std::unexpected(value.error());
                    }
                    array.push_back(std::move(*value));
                    if (auto failure = skip_trivia())
                    {
                        return std::unexpected(*failure);
                    }
                    if (consume("]"))
                    {
                        return DocumentValue{std::move(array)};
                    }
                    if (!consume(","))
                    {
                        return error("expected ',' or ']'");
                    }
                }
            }

            std::expected<DocumentValue, JsonError> parse_object(std::size_t depth)
            {
                ++m_pos;
                DocumentValue::Object object;
                if (auto failure = skip_trivia())
                {
                    return std::unexpected(*failure);
                }
                if (consume("}"))
                {
                    return DocumentValue{std::move(object)};
                }
                while (true)
                {
                    if (peek() != '"')
                    {
                        return error("expected an object key");
                    }
                    auto key = parse_string();
                    if (!key)
                    {
                        return std::unexpected(key.error());
                    }
                    if (auto failure = skip_trivia())
                    {
                        return std::unexpected(*failure);
                    }
                    if (!consume(":"))
                    {
                        return error("expected ':'");
                    }
                    auto value = parse_value(depth);
                    if (!value)
                    {
                        return std::unexpected(value.error());
                    }
                    object.insert_or_assign(std::move(*key), std::move(*value));
                    if (auto failure = skip_trivia())
                    {
                        return std::unexpected(*failure);
                    }
                    if (consume("}"))
                    {
                        return DocumentValue{std::move(object)};
                    }
                    if (!consume(","))
                    {
                        return error("expected ',' or '}'");
                    }
                    if (auto failure = skip_trivia())
                    {
                        return std::unexpected(*failure);
                    }
                }
            }

            std::expected<std::string, JsonError> parse_string()
            {
                ++m_pos;
                std::string text;
                while (m_pos < m_input.size())
                {
                    const auto byte = static_cast<unsigned char>(peek());
                    if (consume("\""))
                    {
                        return text;
                    }
                    if (consume("\\"))
                    {
                        auto code_point = parse_escape();
                        if (!code_point)
                        {
                            return std::unexpected(code_point.error());
                        }
                        append_utf8(text, *code_point);
                    }
                    else if (byte < 0x20)
                    {
                        return error("unescaped control character");
                    }
                    else if (byte < 0x80)
                    {
                        text += m_input[m_pos++];
                    }
                    else
                    {
                        const auto length = detail::utf8_length(m_input.substr(m_pos));
                        if (length == 0)
                        {
                            return error("invalid UTF-8");
                        }
                        text.append(m_input.substr(m_pos, length));
                        m_pos += length;
                    }
                }
                return error("unterminated string");
            }

            std::expected<std::uint32_t, JsonError> parse_escape()
            {
                if (m_pos == m_input.size())
                {
                    return error("incomplete escape");
                }
                switch (m_input[m_pos++])
                {
                case '"':
                    return '"';
                case '\\':
                    return '\\';
                case '/':
                    return '/';
                case 'b':
                    return '\b';
                case 'f':
                    return '\f';
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case 'u':
                    return parse_unicode_escape();
                default:
                    --m_pos;
                    return error("invalid string escape");
                }
            }

            std::expected<std::uint32_t, JsonError> parse_hex_quad()
            {
                std::uint32_t value = 0;
                for (int i = 0; i < 4; ++i)
                {
                    const auto c = peek();
                    const auto lower = c >= 'A' && c <= 'F' ? static_cast<char>(c - 'A' + 'a') : c;
                    const auto digit = hex_digits.find(lower);
                    if (digit == std::string_view::npos)
                    {
                        return error("invalid Unicode escape");
                    }
                    value = (value << 4) | static_cast<std::uint32_t>(digit);
                    ++m_pos;
                }
                return value;
            }

            std::expected<std::uint32_t, JsonError> parse_unicode_escape()
            {
                auto first = parse_hex_quad();
                if (!first || !detail::is_surrogate(*first))
                {
                    return first;
                }
                if (*first >= 0xdc00 || !consume("\\u"))
                {
                    return error("unpaired Unicode surrogate");
                }
                auto second = parse_hex_quad();
                if (!second)
                {
                    return second;
                }
                if (*second < 0xdc00 || *second > 0xdfff)
                {
                    return error("unpaired Unicode surrogate");
                }
                return 0x10000 + ((*first - 0xd800) << 10) + (*second - 0xdc00);
            }
        };

        class Writer
        {
        public:
            explicit Writer(bool pretty)
                : m_pretty{pretty}
            {
            }

            std::expected<std::string, JsonError> write(const DocumentValue &value)
            {
                if (auto result = write_value(value, 0); !result)
                {
                    return std::unexpected(result.error());
                }
                return std::move(m_output);
            }

        private:
            bool m_pretty;
            std::string m_output;

            template <typename Collection>
            std::expected<void, JsonError> write_collection(const Collection &values, std::size_t depth)
            {
                constexpr bool is_object = std::is_same_v<Collection, DocumentValue::Object>;
                if (depth == max_depth)
                {
                    return std::unexpected(JsonError{"nesting limit exceeded", std::nullopt});
                }
                m_output += is_object ? '{' : '[';
                bool first = true;
                for (const auto &item : values)
                {
                    if (!first)
                    {
                        m_output += ',';
                    }
                    first = false;
                    indent(depth + 1);
                    if constexpr (is_object)
                    {
                        if (auto result = write_string(item.first); !result)
                        {
                            return result;
                        }
                        m_output += m_pretty ? ": " : ":";
                        if (auto result = write_value(item.second, depth + 1); !result)
                        {
                            return result;
                        }
                    }
                    else if (auto result = write_value(item, depth + 1); !result)
                    {
                        return result;
                    }
                }
                if (!first)
                {
                    indent(depth);
                }
                m_output += is_object ? '}' : ']';
                return {};
            }

            std::expected<void, JsonError> write_value(const DocumentValue &value, std::size_t depth)
            {
                return std::visit([&](const auto &item) -> std::expected<void, JsonError>
                                  {
                    using T = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<T, std::nullptr_t>)
                    {
                        m_output += "null";
                    }
                    else if constexpr (std::is_same_v<T, bool>)
                    {
                        m_output += item ? "true" : "false";
                    }
                    else if constexpr (std::is_same_v<T, Number>)
                    {
                        std::size_t pos = 0;
                        if (!detail::scan_number(item.m_text, pos) || pos != item.m_text.size())
                        {
                            return std::unexpected(JsonError{"invalid number", std::nullopt});
                        }
                        m_output += item.m_text;
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        return write_string(item);
                    }
                    else
                    {
                        return write_collection(item, depth);
                    }
                    return {}; }, value.m_data);
            }

            void indent(std::size_t depth)
            {
                if (m_pretty)
                {
                    m_output += '\n';
                    m_output.append(depth * 2, ' ');
                }
            }

            std::expected<void, JsonError> write_string(std::string_view text)
            {
                m_output += '"';
                for (std::size_t pos = 0; pos < text.size();)
                {
                    const auto byte = static_cast<unsigned char>(text[pos]);
                    if (byte >= 0x80)
                    {
                        const auto length = detail::utf8_length(text.substr(pos));
                        if (length == 0)
                        {
                            return std::unexpected(JsonError{"invalid UTF-8", std::nullopt});
                        }
                        m_output.append(text.substr(pos, length));
                        pos += length;
                        continue;
                    }
                    ++pos;
                    if (byte == '"' || byte == '\\')
                    {
                        m_output += '\\';
                        m_output += static_cast<char>(byte);
                    }
                    else if (byte < 0x20)
                    {
                        m_output += "\\u00";
                        m_output += hex_digits[byte >> 4];
                        m_output += hex_digits[byte & 0x0f];
                    }
                    else
                    {
                        m_output += static_cast<char>(byte);
                    }
                }
                m_output += '"';
                return {};
            }
        };
    }

    std::expected<DocumentValue, JsonError> parse_json(std::string_view input)
    {
        return Parser{input}.parse();
    }

    std::expected<DocumentValue, JsonError> read_json(std::istream &input)
    {
        try
        {
            if (input.good())
            {
                const std::string text{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
                if (!input.bad())
                {
                    return parse_json(text);
                }
            }
        }
        catch (const std::ios_base::failure &)
        {
            return std::unexpected(JsonError{"failed to read JSON stream", std::nullopt});
        }
        return std::unexpected(JsonError{"failed to read JSON stream", std::nullopt});
    }

    std::expected<std::string, JsonError> write_json(const DocumentValue &value, bool pretty)
    {
        return Writer{pretty}.write(value);
    }

    std::expected<void, JsonError> write_json(std::ostream &output, const DocumentValue &value, bool pretty)
    {
        auto text = write_json(value, pretty);
        if (!text)
        {
            return std::unexpected(text.error());
        }
        if (!std::in_range<std::streamsize>(text->size()))
        {
            return std::unexpected(JsonError{"JSON output exceeds stream size limit", std::nullopt});
        }
        try
        {
            output.write(text->data(), static_cast<std::streamsize>(text->size()));
            if (output)
            {
                return {};
            }
        }
        catch (const std::ios_base::failure &)
        {
            return std::unexpected(JsonError{"failed to write JSON stream", std::nullopt});
        }
        return std::unexpected(JsonError{"failed to write JSON stream", std::nullopt});
    }
}
