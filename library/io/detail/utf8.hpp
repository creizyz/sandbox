#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace io::detail
{
    constexpr bool is_surrogate(std::uint32_t code_point)
    {
        return code_point >= 0xd800 && code_point <= 0xdfff;
    }

    // The input starts with a non-ASCII byte. Zero indicates malformed UTF-8.
    inline std::size_t utf8_length(std::string_view input)
    {
        const auto lead = static_cast<unsigned char>(input.front());
        std::size_t length;
        std::uint32_t code_point;
        std::uint32_t minimum;
        if (lead >= 0xc2 && lead <= 0xdf)
        {
            length = 2;
            code_point = lead & 0x1f;
            minimum = 0x80;
        }
        else if (lead >= 0xe0 && lead <= 0xef)
        {
            length = 3;
            code_point = lead & 0x0f;
            minimum = 0x800;
        }
        else if (lead >= 0xf0 && lead <= 0xf4)
        {
            length = 4;
            code_point = lead & 0x07;
            minimum = 0x10000;
        }
        else
        {
            return 0;
        }
        if (input.size() < length)
        {
            return 0;
        }
        for (std::size_t i = 1; i < length; ++i)
        {
            const auto byte = static_cast<unsigned char>(input[i]);
            if ((byte & 0xc0) != 0x80)
            {
                return 0;
            }
            code_point = (code_point << 6) | (byte & 0x3f);
        }
        if (code_point < minimum || code_point > 0x10ffff || is_surrogate(code_point))
        {
            return 0;
        }
        return length;
    }

    inline bool valid_utf8(std::string_view text)
    {
        while (!text.empty())
        {
            const auto length = static_cast<unsigned char>(text.front()) < 0x80 ? 1 : utf8_length(text);
            if (length == 0)
            {
                return false;
            }
            text.remove_prefix(length);
        }
        return true;
    }
}
