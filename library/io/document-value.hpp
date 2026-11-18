#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <map>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

#include "core/value.hpp"
#include "detail/number.hpp"

namespace io
{
    enum class ValueConversionError
    {
        InvalidNumber,
        NumberOutOfRange,
        NonFiniteNumber,
        NestingLimit,
        InvalidValue
    };

    namespace detail
    {
        template <typename T>
        concept CoreNumber = std::same_as<T, std::int32_t> || std::same_as<T, std::uint32_t> || std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t> || std::same_as<T, char> || std::same_as<T, unsigned char> || std::same_as<T, float> || std::same_as<T, double>;
    }

    struct Number
    {
        enum class Type
        {
            Unspecified,
            Int32,
            UInt32,
            Int64,
            UInt64,
            Char,
            UnsignedChar,
            Float,
            Double
        };

        // A finite decimal number token using JSON number syntax.
        std::string m_text;
        Type m_type{Type::Unspecified};

        template <detail::CoreNumber T>
        [[nodiscard]] static std::expected<Number, ValueConversionError> from(T value)
        {
            if constexpr (std::floating_point<T>)
            {
                if (!std::isfinite(value))
                {
                    return std::unexpected(ValueConversionError::NonFiniteNumber);
                }
            }

            std::array<char, 64> buffer;
            const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), +value);
            if (error != std::errc{})
            {
                return std::unexpected(ValueConversionError::NumberOutOfRange);
            }
            return Number{std::string(buffer.data(), end), type_for<T>()};
        }

        template <detail::CoreNumber T>
        [[nodiscard]] std::expected<T, ValueConversionError> to() const
        {
            std::size_t pos = 0;
            if (!detail::scan_number(m_text, pos) || pos != m_text.size())
            {
                return std::unexpected(ValueConversionError::InvalidNumber);
            }
            if constexpr (std::unsigned_integral<T>)
            {
                if (m_text.starts_with('-'))
                {
                    return std::unexpected(ValueConversionError::NumberOutOfRange);
                }
            }

            constexpr bool is_character = std::same_as<T, char> || std::same_as<T, unsigned char>;
            using Parsed = std::conditional_t<is_character, int, T>;
            Parsed value{};
            const auto *end = m_text.data() + m_text.size();
            const auto [last, error] = std::from_chars(m_text.data(), end, value);
            if (error == std::errc::result_out_of_range)
            {
                return std::unexpected(ValueConversionError::NumberOutOfRange);
            }
            if (error != std::errc{} || last != end)
            {
                return std::unexpected(ValueConversionError::InvalidNumber);
            }
            if constexpr (is_character)
            {
                if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
                {
                    return std::unexpected(ValueConversionError::NumberOutOfRange);
                }
            }
            return static_cast<T>(value);
        }

        // Type hints affect conversions to core Value, while document equality uses the token.
        bool operator==(const Number &other) const
        {
            return m_text == other.m_text;
        }

    private:
        template <detail::CoreNumber T>
        static consteval Type type_for()
        {
            if constexpr (std::same_as<T, std::int32_t>)
            {
                return Type::Int32;
            }
            else if constexpr (std::same_as<T, std::uint32_t>)
            {
                return Type::UInt32;
            }
            else if constexpr (std::same_as<T, std::int64_t>)
            {
                return Type::Int64;
            }
            else if constexpr (std::same_as<T, std::uint64_t>)
            {
                return Type::UInt64;
            }
            else if constexpr (std::same_as<T, char>)
            {
                return Type::Char;
            }
            else if constexpr (std::same_as<T, unsigned char>)
            {
                return Type::UnsignedChar;
            }
            else if constexpr (std::same_as<T, float>)
            {
                return Type::Float;
            }
            else
            {
                return Type::Double;
            }
        }
    };

    struct DocumentValue
    {
        using Array = std::vector<DocumentValue>;
        using Object = std::map<std::string, DocumentValue>;
        using Data = std::variant<std::nullptr_t, bool, Number, std::string, Array, Object>;

        Data m_data{nullptr};

        [[nodiscard]] static std::expected<DocumentValue, ValueConversionError> from_core(const ::Value &value);
        [[nodiscard]] std::expected<::Value, ValueConversionError> to_core() const;

        bool operator==(const DocumentValue &) const = default;
    };
}
