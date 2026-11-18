#pragma once

#include <cstddef>
#include <string_view>

namespace io::detail
{
    constexpr bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    inline bool scan_number(std::string_view input, std::size_t &pos)
    {
        const auto peek = [&]
        { return pos < input.size() ? input[pos] : '\0'; };
        const auto consume_digits = [&]
        {
            const auto start = pos;
            while (is_digit(peek()))
            {
                ++pos;
            }
            return pos != start;
        };

        if (peek() == '-')
        {
            ++pos;
        }
        if (peek() == '0')
        {
            ++pos;
        }
        else if (!consume_digits())
        {
            return false;
        }
        if (peek() == '.')
        {
            ++pos;
            if (!consume_digits())
            {
                return false;
            }
        }
        if (peek() == 'e' || peek() == 'E')
        {
            ++pos;
            if (peek() == '+' || peek() == '-')
            {
                ++pos;
            }
            return consume_digits();
        }
        return true;
    }
}
