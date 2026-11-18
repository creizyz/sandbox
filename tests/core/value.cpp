#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>

#include "core/value.hpp"

static_assert(std::is_same_v<std::variant_alternative_t<10, Value::Data>, Value::Array>);
static_assert(std::is_same_v<std::variant_alternative_t<11, Value::Data>, Value::Object>);
static_assert(std::is_same_v<std::variant_alternative_t<12, Value::Data>, bool>);

TEST(CoreValue, WritesAndReadsTheBooleanTag)
{
    for (const bool flag : {false, true})
    {
        std::stringstream stream;
        Value{flag}.write_binary(stream);
        ASSERT_EQ(stream.str(), (std::string{'\x0c', flag ? '\x01' : '\x00'}));

        const auto decoded = Value::read_binary(stream);
        ASSERT_TRUE(decoded.is<bool>());
        EXPECT_EQ(decoded.as<bool>(), flag);
    }
}

TEST(CoreValue, KeepsLegacyNullAndIntegerBinaryTags)
{
    std::stringstream stream;
    Value{}.write_binary(stream);
    Value{std::int32_t{0}}.write_binary(stream);
    EXPECT_EQ(stream.str(), std::string("\x00\x01\x00\x00\x00\x00", 6));
    EXPECT_TRUE(Value::read_binary(stream).empty());
    const auto decoded = Value::read_binary(stream);
    ASSERT_TRUE(decoded.is<std::int32_t>());
    EXPECT_EQ(decoded.as<std::int32_t>(), 0);
}

TEST(CoreValue, RejectsTruncatedAndInvalidBooleanPayloads)
{
    std::istringstream truncated{std::string("\x0c", 1)};
    std::istringstream invalid{std::string("\x0c\x02", 2)};
    EXPECT_TRUE(Value::read_binary(truncated).empty());
    EXPECT_TRUE(Value::read_binary(invalid).empty());
}
