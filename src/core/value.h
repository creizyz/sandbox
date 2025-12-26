#pragma once

#include <vector>
#include <variant>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "../io/indentation.h"

class Value
{
public:
    using size_t = uint16_t;
    using array_t = std::vector<Value>;
    using object_t = std::unordered_map<std::string, Value>;
    using data_t = std::variant<
        std::monostate,
        int32_t, uint32_t,
        int64_t, uint64_t,
        char, unsigned char,
        float, double,
        std::string,
        array_t,
        object_t
    >;

private:
    data_t m_data;

public:

    Value()
        : m_data(std::monostate{})
    { }

    Value(const Value & other) = default;
    Value(Value && other) noexcept = default;

    Value & operator=(const Value & other) = default;
    Value & operator=(Value && other) noexcept = default;

    static Value array() noexcept { return Value{ array_t{} }; }
    static Value object() noexcept { return Value{ object_t{} }; }

    template <typename T>
        requires (!std::is_same_v<std::decay_t<T>, Value>
                && std::is_constructible_v<data_t, T>)
    explicit Value(T && value)
        : m_data(std::forward<T>(value))
    { }

    template <typename T>
        requires (!std::is_same_v<std::decay_t<T>, Value>
                && std::is_constructible_v<data_t, T>)
    Value & operator=(T && value)
    {
        m_data = std::forward<T>(value);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return std::holds_alternative<std::monostate>(m_data);
    }

    template <typename T>
    [[nodiscard]] bool is() const noexcept
    {
        return std::holds_alternative<T>(m_data);
    }

    template <typename T>
    T& as()
    {
        return std::get<T>(m_data);
    }

    template <typename T>
    const T& as() const
    {
        return std::get<T>(m_data);
    }

    std::string toString() const;

    void write_json(std::ostream & os) const noexcept;
    void write_pretty_json(std::ostream & os, const std::string & indent = "  ") const noexcept;
    static Value read_json(std::istream & is) noexcept;

    void write_binary(std::ostream & os) const noexcept;
    static Value read_binary(std::istream & is) noexcept;

private:
    void write_pretty_json_impl(std::ostream & os, Indentation & indentation) const noexcept;
};
