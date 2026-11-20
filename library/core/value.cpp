#include "value.hpp"

#include <algorithm>
#include <array>
#include <istream>
#include <limits>
#include <ostream>

namespace
{
    template <typename T>
    void write_primitive(std::ostream &output, const T &value)
    {
        output.write(reinterpret_cast<const char *>(&value), sizeof(value));
    }

    template <typename T>
    T read_primitive(std::istream &input)
    {
        T value{};
        input.read(reinterpret_cast<char *>(&value), sizeof(value));
        return value;
    }

    // Old writers used native size_t for object keys. Grow only as bytes arrive.
    std::string read_text(std::istream &input, std::size_t length)
    {
        std::string result;
        std::array<char, 1024> buffer;
        while (length > 0 && input)
        {
            const auto count = std::min(length, buffer.size());
            if (!input.read(buffer.data(), static_cast<std::streamsize>(count)))
            {
                break;
            }
            result.append(buffer.data(), count);
            length -= count;
        }
        return result;
    }

    bool encodable(const Value &value)
    {
        constexpr auto limit = std::numeric_limits<Value::Size>::max();
        if (value.is<std::string>())
        {
            return value.as<std::string>().size() <= limit;
        }
        if (value.is<Value::Array>())
        {
            const auto &array = value.as<Value::Array>();
            return array.size() <= limit && std::ranges::all_of(array, encodable);
        }
        if (value.is<Value::Object>())
        {
            const auto &object = value.as<Value::Object>();
            return object.size() <= limit && std::ranges::all_of(object, [](const auto &entry)
                                                                 { return std::in_range<std::streamsize>(entry.first.size()) && encodable(entry.second); });
        }
        return true;
    }

    template <typename T>
    Value read_numeric(std::istream &input)
    {
        const auto value = read_primitive<T>(input);
        return input ? Value{value} : Value{};
    }
}

void Value::write_binary(std::ostream &output) const
{
    if (!encodable(*this))
    {
        output.setstate(std::ios::failbit);
        return;
    }
    write_binary_unchecked(output);
}

void Value::write_binary_unchecked(std::ostream &output) const
{
    write_primitive(output, static_cast<std::uint8_t>(m_data.index()));
    std::visit([&output](const auto &data)
               {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, bool>)
        {
            write_primitive(output, static_cast<std::uint8_t>(data));
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            write_primitive(output, data);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            write_primitive(output, static_cast<Size>(data.size()));
            output.write(data.data(), static_cast<std::streamsize>(data.size()));
        }
        else if constexpr (std::is_same_v<T, Array> || std::is_same_v<T, Object>)
        {
            write_primitive(output, static_cast<Size>(data.size()));
            for (const auto &item : data)
            {
                if (!output)
                {
                    break;
                }
                if constexpr (std::is_same_v<T, Object>)
                {
                    write_primitive(output, item.first.size());
                    output.write(item.first.data(), static_cast<std::streamsize>(item.first.size()));
                    item.second.write_binary_unchecked(output);
                }
                else
                {
                    item.write_binary_unchecked(output);
                }
            }
        } }, m_data);
}

Value Value::read_binary(std::istream &input)
{
    const auto tag = read_primitive<std::uint8_t>(input);
    if (!input)
    {
        return {};
    }
    switch (tag)
    {
    case 0:
        return {};
    case 1:
        return read_numeric<std::int32_t>(input);
    case 2:
        return read_numeric<std::uint32_t>(input);
    case 3:
        return read_numeric<std::int64_t>(input);
    case 4:
        return read_numeric<std::uint64_t>(input);
    case 5:
        return read_numeric<char>(input);
    case 6:
        return read_numeric<unsigned char>(input);
    case 7:
        return read_numeric<float>(input);
    case 8:
        return read_numeric<double>(input);
    case 9:
    {
        const auto length = read_primitive<Size>(input);
        auto text = read_text(input, length);
        return input ? Value{std::move(text)} : Value{};
    }
    case 10:
    {
        const auto count = read_primitive<Size>(input);
        Array array;
        for (Size i = 0; input && i < count; ++i)
        {
            auto child = read_binary(input);
            if (input)
            {
                array.push_back(std::move(child));
            }
        }
        return input ? Value{std::move(array)} : Value{};
    }
    case 11:
    {
        const auto count = read_primitive<Size>(input);
        Object object;
        for (Size i = 0; input && i < count; ++i)
        {
            const auto key_length = read_primitive<std::size_t>(input);
            auto key = read_text(input, key_length);
            if (!input)
            {
                break;
            }
            auto child = read_binary(input);
            if (input && !object.emplace(std::move(key), std::move(child)).second)
            {
                input.setstate(std::ios::failbit);
            }
        }
        return input ? Value{std::move(object)} : Value{};
    }
    case 12:
    {
        const auto byte = read_primitive<std::uint8_t>(input);
        if (input && byte <= 1)
        {
            return Value{byte == 1};
        }
        break;
    }
    }
    input.setstate(std::ios::failbit);
    return {};
}
