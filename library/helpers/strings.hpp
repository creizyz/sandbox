#pragma once

#include <string>
#include <string_view>

namespace str
{
    std::string unescape(std::string_view sv);
    std::string to_upper(std::string_view sv);
    std::string to_lower(std::string_view sv);
}
