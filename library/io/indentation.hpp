#pragma once

#include <string>
#include <ostream>

class Indentation
{
private:
    std::string m_symbol;
    std::size_t m_level;

public:
    explicit Indentation(const std::string & symbol)
        : m_symbol{ symbol }
        , m_level{ 0 }
    { }

    friend std::ostream & operator<<(std::ostream & os, const Indentation & indent);

    void increase()
    {
        ++m_level;
    }

    void decrease()
    {
        --m_level;
    }

    Indentation & operator++()
    {
        ++m_level;
        return *this;
    }

    Indentation operator++(int)
    {
        auto tmp{ *this };
        ++(*this);
        return tmp;
    }

    Indentation & operator--()
    {
        --m_level;
        return *this;
    }

    Indentation operator--(int)
    {
        auto tmp{ *this };
        --(*this);
        return tmp;
    }
};

inline std::ostream & operator<<(std::ostream & os, const Indentation & indent)
{
    for (auto i = 0u; i < indent.m_level; ++i)
    {
        os << indent.m_symbol;
    }
    return os;
}
