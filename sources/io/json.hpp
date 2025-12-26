#pragma once

#include <cstdint>
#include <string_view>

#include "core/value.hpp"

enum class TokenType : uint8_t
{
    Eof, Error, Null, String, Number,
    LBrace, RBrace, LBracket, RBracket, Colon, Comma
};

struct Token
{
    TokenType type;
    std::string_view text;
    size_t position;
};

class JsonLexer
{
private:
    const char * m_ptr;
    const char * const m_start;
    const char * const m_end;

public:
    explicit JsonLexer(std::string_view source);

    Token next() noexcept;
    char peek() noexcept;

private:
    size_t remaining() const { return static_cast<size_t>(m_end - m_ptr); }
    size_t position() const { return static_cast<size_t>(m_ptr - m_start); }

    void skipWhitespace() noexcept;

    Token scanString() noexcept;
    Token scanNumber() noexcept;
};

class JsonParser
{
public:
    Value parse(std::string_view sv);

private:
    Value parseRecursive(JsonLexer & lexer);
    Value parseArray(JsonLexer & lexer);
    Value parseObject(JsonLexer & lexer);
    Value parseArithmeticWrapper(JsonLexer & lexer);
};
