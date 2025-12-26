#include "value.hpp"

#include <array>
#include <sstream>

std::string Value::toString() const
{
    std::ostringstream ss;
    write_json(ss);
    return ss.str();
}

constexpr std::array<std::string_view, std::variant_size_v<Value::data_t>> value_type_names
{
    "null",
    "int32",
    "uint32",
    "int64",
    "uint64",
    "char",
    "uchar",
    "float",
    "double",
    "string",
    "array",
    "object"
};

void Value::write_json(std::ostream & os) const noexcept
{
    const auto index = static_cast<uint8_t>(m_data.index());

    std::visit([&os, &index](auto && arg)
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            os << "null";
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            os << R"({ "type": ")" << value_type_names[index] << R"(", "value": )" << arg << " }";
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            os << "\"" << arg << "\"";
        }
        else if constexpr (std::is_same_v<T, array_t>)
        {
            os << "[ ";
            for (const auto & item : arg)
            {
                item.write_json(os);
            }
            os << " ]";
        }
        else if constexpr (std::is_same_v<T, object_t>)
        {
            os << "{ ";
            for (auto it = arg.begin(); it != arg.end(); ++it)
            {
                const auto & [key, value] = *it;
                os << "\"" << key << "\": ";
                value.write_json(os);
                if (std::next(it) != arg.end())
                {
                    os << ", ";
                }
            }
            os << " }";
        }
    }, m_data);
}

void Value::write_pretty_json(std::ostream & os, const std::string & indent) const noexcept
{
    Indentation indentation{ indent };
    write_pretty_json_impl(os, indentation);
}

void Value::write_pretty_json_impl(std::ostream & os, Indentation & indentation) const noexcept
{
    const auto index = static_cast<uint8_t>(m_data.index());

    std::visit([&os, &index, &indentation](auto && arg)
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            os << "null";
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            os << "{\n";
            indentation.increase();
            os << indentation << "\"type\": \"" << value_type_names[index] << "\",\n";
            os << indentation << "\"value\": " << arg << "\n";
            indentation.decrease();
            os << " }";
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            os << "\"" << arg << "\"";
        }
        else if constexpr (std::is_same_v<T, array_t>)
        {
            os << "[ ";
            indentation.increase();
            for (const auto & item : arg)
            {
                item.write_pretty_json_impl(os, indentation);
            }
            indentation.decrease();
            os << " ]";
        }
        else if constexpr (std::is_same_v<T, object_t>)
        {
            os << "{ ";
            indentation.increase();
            for (auto it = arg.begin(); it != arg.end(); ++it)
            {
                const auto & [key, value] = *it;
                os << indentation << "\"" << key << "\": ";
                value.write_pretty_json_impl(os, indentation);
                if (std::next(it) != arg.end())
                {
                    os << ",\n";
                }
            }
            indentation.decrease();
            os << " }";
        }
    }, m_data);
}

Value Value::read_json(std::istream & is) noexcept
{

}

void Value::write_binary(std::ostream & os) const noexcept
{
    const auto index = static_cast<uint8_t>(m_data.index());
    os.write(reinterpret_cast<const char*>(&index), sizeof(index));

    std::visit([&os](auto && arg)
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
        {
            return; // nothing to write
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            os.write(reinterpret_cast<const char*>(&arg), sizeof(T));
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            const auto size = static_cast<Value::size_t>(arg.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));
            os.write(arg.data(), size);
        }
        else if constexpr (std::is_same_v<T, array_t>)
        {
            const auto size = static_cast<Value::size_t>(arg.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));
            for (const auto & item : arg)
            {
                item.write_binary(os);
            }
        }
        else if constexpr (std::is_same_v<T, object_t>)
        {
            const auto size = static_cast<Value::size_t>(arg.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));
            for (const auto & [key, value] : arg)
            {
                const auto key_size = key.size();
                os.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
                os.write(key.data(), key_size);
                value.write_binary(os);
            }
        }
    }, m_data);
}

template <typename T>
    requires (std::is_arithmetic_v<T>)
 T read_primitive(std::istream & is)
{
    T value{ };
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

inline std::string read_string(std::istream & is)
{
    const auto size = read_primitive<Value::size_t>(is);
    std::string s(size, '\0');
    is.read(&s[0], size);
    return s;
}

Value Value::read_binary(std::istream & is) noexcept
{
    Value value;
    uint8_t index = 0;

    if (!is.read(reinterpret_cast<char*>(&index), sizeof(index)))
    {
        return value;
    }

    switch (index)
    {
        case 0: value.m_data = std::monostate{}; break;
        case 1: value.m_data = read_primitive<int32_t>(is); break;
        case 2: value.m_data = read_primitive<uint32_t>(is); break;
        case 3: value.m_data = read_primitive<int64_t>(is); break;
        case 4: value.m_data = read_primitive<uint64_t>(is); break;
        case 5: value.m_data = read_primitive<char>(is); break;
        case 6: value.m_data = read_primitive<unsigned char>(is); break;
        case 7: value.m_data = read_primitive<float>(is); break;
        case 8: value.m_data = read_primitive<double>(is); break;
        case 9: value.m_data = read_string(is); break;
        case 10:
        { // array_t
            const auto size = read_primitive<Value::size_t>(is);
            array_t arr;
            arr.reserve(size);
            for (auto i = 0u; i < size; ++i)
            {
                arr.push_back(read_binary(is));
            }
            value.m_data = std::move(arr);
            break;
        }
        case 11:
        { // object_t
            const auto size = read_primitive<Value::size_t>(is);
            object_t obj;
            obj.reserve(size);
            for (auto i = 0u; i < size; ++i)
            {
                const auto key = read_string(is);
                obj.emplace(key, read_binary(is));
            }
            value.m_data = std::move(obj);
            break;
        }
        default: /* do nothing */ break;
    }

    return value;
}

