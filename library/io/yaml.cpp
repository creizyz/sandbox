#include "yaml.hpp"

#include <charconv>
#include <cstdint>
#include <istream>
#include <iterator>
#include <ostream>
#include <type_traits>
#include <utility>

#include <yaml.h>

#include "detail/number.hpp"
#include "detail/utf8.hpp"

namespace io
{
    namespace
    {
        constexpr std::size_t max_depth = 256;

        template <typename T>
        using Result = std::expected<T, YamlError>;

        YamlError at(std::string_view message, yaml_mark_t mark)
        {
            return {std::string(message), YamlLocation{mark.line + 1, mark.column + 1}};
        }

        // LibYAML reader errors identify a byte offset, before the scanner has updated its mark.
        YamlLocation reader_location(std::string_view input, std::size_t offset, yaml_encoding_t encoding)
        {
            auto prefix = input.substr(0, offset);
            const bool utf16 = encoding == YAML_UTF16LE_ENCODING || encoding == YAML_UTF16BE_ENCODING;
            if (utf16 && prefix.size() >= 2)
            {
                prefix.remove_prefix(2); // LibYAML detects UTF-16 by its BOM.
            }
            else if (prefix.starts_with("\xef\xbb\xbf"))
            {
                prefix.remove_prefix(3);
            }

            YamlLocation location{1, 1};
            bool after_cr = false;
            while (!prefix.empty())
            {
                unsigned code = static_cast<unsigned char>(prefix.front());
                std::size_t width = 1;
                if (utf16)
                {
                    if (prefix.size() < 2)
                    {
                        break;
                    }
                    const auto second = static_cast<unsigned char>(prefix[1]);
                    code = encoding == YAML_UTF16LE_ENCODING ? code | (second << 8) : (code << 8) | second;
                    width = code >= 0xd800 && code <= 0xdbff ? 4 : 2;
                }
                else if (code >= 0x80)
                {
                    width = detail::utf8_length(prefix);
                    if (prefix.starts_with("\xc2\x85"))
                    {
                        code = 0x85;
                    }
                    else if (prefix.starts_with("\xe2\x80\xa8") || prefix.starts_with("\xe2\x80\xa9"))
                    {
                        code = 0x2028;
                    }
                }
                if (width == 0 || width > prefix.size())
                {
                    break;
                }
                prefix.remove_prefix(width);
                if (code == '\r' || code == '\n' || code == 0x85 || code == 0x2028 || code == 0x2029)
                {
                    if (!(after_cr && code == '\n'))
                    {
                        ++location.m_line;
                    }
                    location.m_column = 1;
                }
                else
                {
                    ++location.m_column;
                }
                after_cr = code == '\r';
            }
            return location;
        }

        std::unexpected<YamlError> output_error(std::string_view message)
        {
            return std::unexpected(YamlError{std::string(message), std::nullopt});
        }

        std::string_view tag_text(const yaml_char_t *tag)
        {
            return tag ? std::string_view(reinterpret_cast<const char *>(tag)) : std::string_view{};
        }

        struct ScalarNumber
        {
            Number m_number;
            bool m_integer;
        };

        Result<std::optional<ScalarNumber>> resolve_number(std::string_view text, yaml_mark_t mark)
        {
            const auto first = text.front();
            if (first != '+' && first != '-' && first != '.' && !detail::is_digit(first))
            {
                return std::nullopt;
            }
            const auto unsigned_text = first == '+' || first == '-' ? text.substr(1) : text;
            if (unsigned_text == ".inf" || unsigned_text == ".Inf" || unsigned_text == ".INF" || text == ".nan" || text == ".NaN" || text == ".NAN")
            {
                return std::unexpected(at("non-finite YAML numbers are unsupported", mark));
            }
            if (text.starts_with("0o") || text.starts_with("0x"))
            {
                std::uint64_t value;
                const auto digits = text.substr(2);
                const auto end = digits.data() + digits.size();
                const auto [last, error] = std::from_chars(digits.data(), end, value, text[1] == 'o' ? 8 : 16);
                if (error == std::errc::result_out_of_range && last == end)
                {
                    return std::unexpected(at("octal or hexadecimal number exceeds uint64 range", mark));
                }
                if (error != std::errc{} || last != end)
                {
                    return std::nullopt;
                }
                return ScalarNumber{Number{std::to_string(value)}, true};
            }

            auto end_digits = unsigned_text.find_first_not_of("0123456789");
            if (end_digits == std::string_view::npos)
            {
                end_digits = unsigned_text.size();
            }
            auto digits = unsigned_text.substr(0, end_digits);
            const auto rest = unsigned_text.substr(end_digits);
            if (digits.empty() && (!rest.starts_with('.') || rest.size() < 2 || !detail::is_digit(rest[1])))
            {
                return std::nullopt;
            }
            const auto significant = digits.find_first_not_of('0');
            digits = significant == std::string_view::npos ? std::string_view{"0"} : digits.substr(significant);
            std::string token = first == '-' ? "-" : "";
            token += digits;
            if (rest.starts_with('.') && (rest.size() == 1 || rest[1] == 'e' || rest[1] == 'E'))
            {
                token += ".0";
                token += rest.substr(1);
            }
            else
            {
                token += rest;
            }
            std::size_t pos = 0;
            if (!detail::scan_number(token, pos) || pos != token.size())
            {
                return std::nullopt;
            }
            return ScalarNumber{Number{std::move(token)}, rest.empty()};
        }

        Result<DocumentValue> resolve_scalar(const yaml_event_t &event)
        {
            const auto &scalar = event.data.scalar;
            const auto tag = tag_text(scalar.tag);
            const std::string_view text{reinterpret_cast<const char *>(scalar.value), scalar.length};
            const auto mark = event.start_mark;
            if (!detail::valid_utf8(text))
            {
                return std::unexpected(at("invalid UTF-8", mark));
            }
            if (scalar.anchor)
            {
                return std::unexpected(at("YAML anchors are unsupported", mark));
            }
            if (tag == YAML_STR_TAG || tag == "!" || (tag.empty() && scalar.style != YAML_PLAIN_SCALAR_STYLE))
            {
                return DocumentValue{std::string(text)};
            }

            const bool implicit = tag.empty();
            const bool is_null = text.empty() || text == "~" || text == "null" || text == "Null" || text == "NULL";
            if ((implicit || tag == YAML_NULL_TAG) && is_null)
            {
                return DocumentValue{};
            }
            if (implicit || tag == YAML_BOOL_TAG)
            {
                if (text == "true" || text == "True" || text == "TRUE")
                {
                    return DocumentValue{true};
                }
                if (text == "false" || text == "False" || text == "FALSE")
                {
                    return DocumentValue{false};
                }
            }
            if (!text.empty() && (implicit || tag == YAML_INT_TAG || tag == YAML_FLOAT_TAG))
            {
                auto number = resolve_number(text, mark);
                if (!number)
                {
                    return std::unexpected(std::move(number.error()));
                }
                if (*number && (tag != YAML_INT_TAG || (*number)->m_integer))
                {
                    return DocumentValue{std::move((*number)->m_number)};
                }
            }
            if (implicit)
            {
                return DocumentValue{std::string(text)};
            }
            return std::unexpected(at("unsupported YAML tag or invalid tagged scalar", mark));
        }

        class Reader
        {
        public:
            Reader()
                : m_ready{yaml_parser_initialize(&m_parser) != 0}
            {
            }

            ~Reader()
            {
                clear_event();
                if (m_ready)
                {
                    yaml_parser_delete(&m_parser);
                }
            }

            Reader(const Reader &) = delete;
            Reader &operator=(const Reader &) = delete;

            Result<DocumentValue> read(std::string_view input)
            {
                m_input = input;
                if (!m_ready)
                {
                    return output_error("could not initialize YAML parser");
                }
                // LibYAML requires a non-null buffer, even for empty input.
                const auto *data = reinterpret_cast<const unsigned char *>(input.empty() ? "" : input.data());
                yaml_parser_set_input_string(&m_parser, data, input.size());
                if (!next() || m_event.type != YAML_STREAM_START_EVENT || !next() || m_event.type != YAML_DOCUMENT_START_EVENT)
                {
                    return failure("expected a YAML document");
                }
                const auto *version = m_event.data.document_start.version_directive;
                if (version && (version->major != 1 || version->minor != 2))
                {
                    return failure("expected YAML version 1.2");
                }
                if (!next())
                {
                    return failure("expected a YAML value");
                }
                auto result = read_value(0);
                if (!result)
                {
                    return result;
                }
                if (!next() || m_event.type != YAML_DOCUMENT_END_EVENT || !next() || m_event.type != YAML_STREAM_END_EVENT)
                {
                    return failure("expected exactly one YAML document");
                }
                return result;
            }

        private:
            std::string_view m_input;
            yaml_parser_t m_parser{};
            yaml_event_t m_event{};
            bool m_ready;
            bool m_has_event = false;

            void clear_event()
            {
                if (m_has_event)
                {
                    yaml_event_delete(&m_event);
                    m_has_event = false;
                }
            }

            bool next()
            {
                clear_event();
                m_has_event = yaml_parser_parse(&m_parser, &m_event) != 0;
                return m_has_event;
            }

            std::unexpected<YamlError> failure(std::string_view message) const
            {
                if (m_parser.error == YAML_READER_ERROR)
                {
                    return std::unexpected(YamlError{m_parser.problem, reader_location(m_input, m_parser.problem_offset, m_parser.encoding)});
                }
                if (m_parser.error != YAML_NO_ERROR)
                {
                    return std::unexpected(at(m_parser.problem ? m_parser.problem : "invalid YAML", m_parser.problem_mark));
                }
                return std::unexpected(at(message, m_event.start_mark));
            }

            // Leaves the current event on the scalar or the container's closing event.
            Result<DocumentValue> read_value(std::size_t depth)
            {
                if (m_event.type == YAML_SCALAR_EVENT)
                {
                    return resolve_scalar(m_event);
                }
                if (m_event.type == YAML_ALIAS_EVENT)
                {
                    return failure("YAML aliases are unsupported");
                }
                if (m_event.type != YAML_SEQUENCE_START_EVENT && m_event.type != YAML_MAPPING_START_EVENT)
                {
                    return failure("expected a YAML value");
                }
                if (depth == max_depth)
                {
                    return failure("nesting limit exceeded");
                }
                const bool sequence = m_event.type == YAML_SEQUENCE_START_EVENT;
                const auto *anchor = sequence ? m_event.data.sequence_start.anchor : m_event.data.mapping_start.anchor;
                const auto tag = tag_text(sequence ? m_event.data.sequence_start.tag : m_event.data.mapping_start.tag);
                if (anchor || (!tag.empty() && tag != "!" && tag != (sequence ? YAML_SEQ_TAG : YAML_MAP_TAG)))
                {
                    return failure("YAML anchors and custom collection tags are unsupported");
                }
                return sequence ? read_array(depth + 1) : read_object(depth + 1);
            }

            Result<DocumentValue> read_array(std::size_t depth)
            {
                DocumentValue::Array array;
                while (next())
                {
                    if (m_event.type == YAML_SEQUENCE_END_EVENT)
                    {
                        return Result<DocumentValue>{std::in_place, std::move(array)};
                    }
                    auto value = read_value(depth);
                    if (!value)
                    {
                        return value;
                    }
                    array.push_back(std::move(*value));
                }
                return failure("expected the end of a YAML sequence");
            }

            Result<DocumentValue> read_object(std::size_t depth)
            {
                DocumentValue::Object object;
                while (next())
                {
                    if (m_event.type == YAML_MAPPING_END_EVENT)
                    {
                        return Result<DocumentValue>{std::in_place, std::move(object)};
                    }
                    if (m_event.type != YAML_SCALAR_EVENT)
                    {
                        return failure("YAML mapping keys must be strings");
                    }
                    auto key = resolve_scalar(m_event);
                    if (!key)
                    {
                        return key;
                    }
                    auto *name = std::get_if<std::string>(&key->m_data);
                    if (!name)
                    {
                        return failure("YAML mapping keys must be strings");
                    }
                    if (object.contains(*name))
                    {
                        return failure("duplicate YAML mapping key");
                    }
                    if (!next())
                    {
                        return failure("expected a YAML mapping value");
                    }
                    auto value = read_value(depth);
                    if (!value)
                    {
                        return value;
                    }
                    object.emplace(std::move(*name), std::move(*value));
                }
                return failure("expected the end of a YAML mapping");
            }
        };

        class Writer
        {
        public:
            Writer()
                : m_ready{yaml_emitter_initialize(&m_emitter) != 0}
            {
            }

            ~Writer()
            {
                if (m_ready)
                {
                    yaml_emitter_delete(&m_emitter);
                }
            }

            Writer(const Writer &) = delete;
            Writer &operator=(const Writer &) = delete;

            Result<std::string> write(const DocumentValue &value)
            {
                if (!m_ready)
                {
                    return output_error("could not initialize YAML emitter");
                }
                yaml_emitter_set_output(&m_emitter, append, &m_text);
                yaml_emitter_set_unicode(&m_emitter, 1);
                if (!emit(yaml_stream_start_event_initialize, YAML_UTF8_ENCODING) || !emit(yaml_document_start_event_initialize, nullptr, nullptr, nullptr, 1))
                {
                    return failure();
                }
                if (auto written = write_value(value, 0); !written)
                {
                    return std::unexpected(std::move(written.error()));
                }
                if (!emit(yaml_document_end_event_initialize, 1) || !emit(yaml_stream_end_event_initialize))
                {
                    return failure();
                }
                return std::move(m_text);
            }

        private:
            yaml_emitter_t m_emitter{};
            bool m_ready;
            std::string m_text;

            static int append(void *data, unsigned char *bytes, std::size_t size) noexcept
            {
                try
                {
                    static_cast<std::string *>(data)->append(reinterpret_cast<const char *>(bytes), size);
                    return 1;
                }
                catch (...)
                {
                    return 0;
                }
            }

            template <typename Initialize, typename... Args>
            bool emit(Initialize initialize, Args... args)
            {
                yaml_event_t event{};
                return initialize(&event, args...) != 0 && yaml_emitter_emit(&m_emitter, &event) != 0;
            }

            std::unexpected<YamlError> failure() const
            {
                return output_error(m_emitter.problem ? m_emitter.problem : "could not write YAML event");
            }

            Result<void> write_scalar(std::string_view text, yaml_scalar_style_t style)
            {
                if (!std::in_range<int>(text.size()))
                {
                    return output_error("YAML scalar exceeds size limit");
                }
                if (!detail::valid_utf8(text))
                {
                    return output_error("invalid UTF-8");
                }
                const auto *data = reinterpret_cast<const unsigned char *>(text.data());
                if (!emit(yaml_scalar_event_initialize, nullptr, nullptr, data, static_cast<int>(text.size()), 1, 1, style))
                {
                    return failure();
                }
                return {};
            }

            Result<void> write_value(const DocumentValue &value, std::size_t depth)
            {
                return std::visit([&](const auto &item) -> Result<void>
                                  {
                    using T = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<T, std::nullptr_t>)
                    {
                        return write_scalar("null", YAML_PLAIN_SCALAR_STYLE);
                    }
                    else if constexpr (std::is_same_v<T, bool>)
                    {
                        return write_scalar(item ? "true" : "false", YAML_PLAIN_SCALAR_STYLE);
                    }
                    else if constexpr (std::is_same_v<T, Number>)
                    {
                        std::size_t pos = 0;
                        if (!detail::scan_number(item.m_text, pos) || pos != item.m_text.size())
                        {
                            return output_error("invalid number");
                        }
                        return write_scalar(item.m_text, YAML_PLAIN_SCALAR_STYLE);
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        return write_scalar(item, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
                    }
                    else
                    {
                        if (depth == max_depth)
                        {
                            return output_error("nesting limit exceeded");
                        }
                        return write_collection(item, depth + 1);
                    } }, value.m_data);
            }

            Result<void> write_collection(const DocumentValue::Array &values, std::size_t depth)
            {
                if (!emit(yaml_sequence_start_event_initialize, nullptr, nullptr, 1, YAML_BLOCK_SEQUENCE_STYLE))
                {
                    return failure();
                }
                for (const auto &value : values)
                {
                    if (auto written = write_value(value, depth); !written)
                    {
                        return written;
                    }
                }
                if (!emit(yaml_sequence_end_event_initialize))
                {
                    return failure();
                }
                return {};
            }

            Result<void> write_collection(const DocumentValue::Object &values, std::size_t depth)
            {
                if (!emit(yaml_mapping_start_event_initialize, nullptr, nullptr, 1, YAML_BLOCK_MAPPING_STYLE))
                {
                    return failure();
                }
                for (const auto &[key, value] : values)
                {
                    if (auto written = write_scalar(key, YAML_DOUBLE_QUOTED_SCALAR_STYLE); !written)
                    {
                        return written;
                    }
                    if (auto written = write_value(value, depth); !written)
                    {
                        return written;
                    }
                }
                if (!emit(yaml_mapping_end_event_initialize))
                {
                    return failure();
                }
                return {};
            }
        };
    }

    Result<DocumentValue> parse_yaml(std::string_view input)
    {
        return Reader{}.read(input);
    }

    Result<DocumentValue> read_yaml(std::istream &input)
    {
        try
        {
            if (input.good())
            {
                const std::string text{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
                if (!input.bad())
                {
                    return parse_yaml(text);
                }
            }
        }
        catch (const std::ios_base::failure &)
        {
            return output_error("failed to read YAML stream");
        }
        return output_error("failed to read YAML stream");
    }

    Result<std::string> write_yaml(const DocumentValue &value)
    {
        return Writer{}.write(value);
    }

    Result<void> write_yaml(std::ostream &output, const DocumentValue &value)
    {
        auto text = write_yaml(value);
        if (!text)
        {
            return std::unexpected(std::move(text.error()));
        }
        if (!std::in_range<std::streamsize>(text->size()))
        {
            return output_error("YAML output exceeds stream size limit");
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
            return output_error("failed to write YAML stream");
        }
        return output_error("failed to write YAML stream");
    }
}
