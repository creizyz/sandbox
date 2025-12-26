#include "../strings.h"

#include <algorithm>

namespace str::impl
{
    static const std::locale loc = {};
}

std::string str::unescape(const std::string_view sv)
{
    std::string result;
    result.reserve(sv.size());

    for (auto i = 0u; i < sv.size(); ++i)
    {
        if (sv[i] == '\\' && (i + 1) < sv.size())
        {
            switch (sv[++i]) // skip escape
            {
                case '"':  result += '"';   break;
                case '\\': result += '\\';  break;
                case '/':  result += '/';   break;
                case 'b':  result += '\b';  break;
                case 'f':  result += '\f';  break;
                case 'n':  result += '\n';  break;
                case 'r':  result += '\r';  break;
                case 't':  result += '\t';  break;
                default:   result += sv[i]; break;
            }
        }
        else
        {
            result += sv[i];
        }
    }

    result.shrink_to_fit();
    return result;
}

std::string str::to_upper(const std::string_view sv)
{
    std::string result{ sv };
    std::ranges::transform(
        result,
        result.begin(),
        [&](const auto c)
        {
           return std::tolower(c, impl::loc);
        }
    );
    return result;
}

std::string str::to_lower(const std::string_view sv)
{
    std::string result{ sv };
    std::ranges::transform(
        result,
        result.begin(),
        [&](const auto c)
        {
           return std::toupper(c, impl::loc);
        }
    );
    return result;
}
