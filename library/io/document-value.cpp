#include "document-value.hpp"

#include <utility>

namespace io
{
    namespace
    {
        constexpr std::size_t max_depth = 256;

        template <typename T>
        using Result = std::expected<T, ValueConversionError>;

        template <std::size_t Index = 0>
        Result<DocumentValue> from_core(const ::Value &value, std::size_t depth);

        template <typename T>
        Result<DocumentValue> from_core_data(const T &item, std::size_t depth)
        {
            if constexpr (std::same_as<T, std::monostate>)
            {
                return DocumentValue{};
            }
            else if constexpr (std::same_as<T, std::string> || std::same_as<T, bool>)
            {
                return Result<DocumentValue>{std::in_place, item};
            }
            else if constexpr (detail::CoreNumber<T>)
            {
                return Number::from(item).transform([](Number &&number)
                                                    { return DocumentValue{std::move(number)}; });
            }
            else
            {
                if (depth == max_depth)
                {
                    return std::unexpected(ValueConversionError::NestingLimit);
                }
                if constexpr (std::same_as<T, ::Value::Array>)
                {
                    DocumentValue::Array array;
                    array.reserve(item.size());
                    for (const auto &child : item)
                    {
                        auto converted = from_core(child, depth + 1);
                        if (!converted)
                        {
                            return converted;
                        }
                        array.push_back(std::move(*converted));
                    }
                    return Result<DocumentValue>{std::in_place, std::move(array)};
                }
                else
                {
                    static_assert(std::same_as<T, ::Value::Object>);
                    DocumentValue::Object object;
                    for (const auto &[key, child] : item)
                    {
                        auto converted = from_core(child, depth + 1);
                        if (!converted)
                        {
                            return converted;
                        }
                        object.emplace(key, std::move(*converted));
                    }
                    return Result<DocumentValue>{std::in_place, std::move(object)};
                }
            }
        }

        template <std::size_t Index>
        Result<DocumentValue> from_core(const ::Value &value, std::size_t depth)
        {
            using T = std::variant_alternative_t<Index, ::Value::Data>;
            if (value.is<T>())
            {
                return from_core_data(value.as<T>(), depth);
            }
            if constexpr (Index + 1 < std::variant_size_v<::Value::Data>)
            {
                return from_core<Index + 1>(value, depth);
            }
            return std::unexpected(ValueConversionError::InvalidValue);
        }

        template <detail::CoreNumber T>
        Result<::Value> to_core_number(const Number &number)
        {
            return number.to<T>().transform([](T value)
                                            { return ::Value{value}; });
        }

        Result<::Value> to_core_number(const Number &number)
        {
            switch (number.m_type)
            {
            case Number::Type::Int32:
                return to_core_number<std::int32_t>(number);
            case Number::Type::UInt32:
                return to_core_number<std::uint32_t>(number);
            case Number::Type::Int64:
                return to_core_number<std::int64_t>(number);
            case Number::Type::UInt64:
                return to_core_number<std::uint64_t>(number);
            case Number::Type::Char:
                return to_core_number<char>(number);
            case Number::Type::UnsignedChar:
                return to_core_number<unsigned char>(number);
            case Number::Type::Float:
                return to_core_number<float>(number);
            case Number::Type::Double:
                return to_core_number<double>(number);
            case Number::Type::Unspecified:
                break;
            }

            if (number.m_text.find_first_of(".eE") != std::string::npos)
            {
                return to_core_number<double>(number);
            }
            auto signed_value = to_core_number<std::int64_t>(number);
            if (!signed_value && signed_value.error() == ValueConversionError::NumberOutOfRange && !number.m_text.starts_with('-'))
            {
                return to_core_number<std::uint64_t>(number);
            }
            return signed_value;
        }

        Result<::Value> to_core(const DocumentValue &value, std::size_t depth)
        {
            if (value.m_data.valueless_by_exception())
            {
                return std::unexpected(ValueConversionError::InvalidValue);
            }
            return std::visit([depth](const auto &item) -> Result<::Value>
                              {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::same_as<T, std::nullptr_t>)
                {
                    return ::Value{};
                }
                else if constexpr (std::same_as<T, std::string> || std::same_as<T, bool>)
                {
                    return Result<::Value>{std::in_place, item};
                }
                else if constexpr (std::same_as<T, Number>)
                {
                    return to_core_number(item);
                }
                else
                {
                    if (depth == max_depth)
                    {
                        return std::unexpected(ValueConversionError::NestingLimit);
                    }
                    if constexpr (std::same_as<T, DocumentValue::Array>)
                    {
                        ::Value::Array array;
                        array.reserve(item.size());
                        for (const auto &child : item)
                        {
                            auto converted = to_core(child, depth + 1);
                            if (!converted)
                            {
                                return converted;
                            }
                            array.push_back(std::move(*converted));
                        }
                        return Result<::Value>{std::in_place, std::move(array)};
                    }
                    else
                    {
                        static_assert(std::same_as<T, DocumentValue::Object>);
                        ::Value::Object object;
                        object.reserve(item.size());
                        for (const auto &[key, child] : item)
                        {
                            auto converted = to_core(child, depth + 1);
                            if (!converted)
                            {
                                return converted;
                            }
                            object.emplace(key, std::move(*converted));
                        }
                        return Result<::Value>{std::in_place, std::move(object)};
                    }
                } }, value.m_data);
        }
    }

    std::expected<DocumentValue, ValueConversionError> DocumentValue::from_core(const ::Value &value)
    {
        return io::from_core(value, 0);
    }

    std::expected<::Value, ValueConversionError> DocumentValue::to_core() const
    {
        return io::to_core(*this, 0);
    }
}
