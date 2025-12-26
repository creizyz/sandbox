#pragma once

#include <string>
#include <locale>

namespace str
{
    std::string unescape(std::string_view sv);
    std::string to_upper(std::string_view sv);
    std::string to_lower(std::string_view sv);
}
