#include "value.hpp"

#include <istream>
#include <ostream>

void Value::write_binary(std::ostream &os) const noexcept
{
    const auto index = static_cast<uint8_t>(m_data.index());
    os.write(reinterpret_cast<const char *>(&index), sizeof(index));

    std::visit([&os](auto &&arg)
               {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            return; // nothing to write
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            const uint8_t byte = arg ? 1 : 0;
            os.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            os.write(reinterpret_cast<const char*>(&arg), sizeof(T));
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            const auto size = static_cast<Value::Size>(arg.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));
            os.write(arg.data(), size);
        }
        else if constexpr (std::is_same_v<T, Array>)
        {
            const auto size = static_cast<Value::Size>(arg.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));
            for (const auto & item : arg)
            {
                item.write_binary(os);
            }
        }
        else if constexpr (std::is_same_v<T, Object>)
        {
            const auto size = static_cast<Value::Size>(arg.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));
            for (const auto & [key, value] : arg)
            {
                const auto key_size = key.size();
                os.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
                os.write(key.data(), key_size);
                value.write_binary(os);
            }
        } }, m_data);
}

template <typename T>
    requires(std::is_arithmetic_v<T>)
T read_primitive(std::istream &is)
{
    T value{};
    is.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

inline std::string read_string(std::istream &is)
{
    const auto size = read_primitive<Value::Size>(is);
    std::string s(size, '\0');
    is.read(&s[0], size);
    return s;
}

Value Value::read_binary(std::istream &is) noexcept
{
    Value value;
    uint8_t index = 0;

    if (!is.read(reinterpret_cast<char *>(&index), sizeof(index)))
    {
        return value;
    }

    switch (index)
    {
    case 0:
        value.m_data = std::monostate{};
        break;
    case 1:
        value.m_data = read_primitive<int32_t>(is);
        break;
    case 2:
        value.m_data = read_primitive<uint32_t>(is);
        break;
    case 3:
        value.m_data = read_primitive<int64_t>(is);
        break;
    case 4:
        value.m_data = read_primitive<uint64_t>(is);
        break;
    case 5:
        value.m_data = read_primitive<char>(is);
        break;
    case 6:
        value.m_data = read_primitive<unsigned char>(is);
        break;
    case 7:
        value.m_data = read_primitive<float>(is);
        break;
    case 8:
        value.m_data = read_primitive<double>(is);
        break;
    case 9:
        value.m_data = read_string(is);
        break;
    case 10:
    { // Array
        const auto size = read_primitive<Value::Size>(is);
        Array arr;
        arr.reserve(size);
        for (auto i = 0u; i < size; ++i)
        {
            arr.push_back(read_binary(is));
        }
        value.m_data = std::move(arr);
        break;
    }
    case 11:
    { // Object
        const auto size = read_primitive<Value::Size>(is);
        Object obj;
        obj.reserve(size);
        for (auto i = 0u; i < size; ++i)
        {
            const auto key = read_string(is);
            obj.emplace(key, read_binary(is));
        }
        value.m_data = std::move(obj);
        break;
    }
    case 12:
    {
        uint8_t byte = 0;
        if (is.read(reinterpret_cast<char *>(&byte), sizeof(byte)) && byte <= 1)
        {
            value.m_data = byte == 1;
        }
        break;
    }
    default: /* do nothing */
        break;
    }

    return value;
}
