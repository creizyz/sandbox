#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
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

TEST(CoreValue, ReadsLegacyNativeWidthObjectKeysAndPreservesFraming)
{
    const Value::Size count = 1;
    const std::size_t length = 4;
    std::string bytes(1, '\x0b');
    bytes.append(reinterpret_cast<const char *>(&count), sizeof(count));
    bytes.append(reinterpret_cast<const char *>(&length), sizeof(length));
    bytes += "name";
    bytes.append("\x0c\x01", 2);

    std::stringstream encoded;
    Value{Value::Object{{"name", Value{true}}}}.write_binary(encoded);
    ASSERT_EQ(encoded.str(), bytes);

    std::istringstream input(bytes + std::string(1, '\x00'));
    auto object = Value::read_binary(input);
    ASSERT_TRUE(input);
    ASSERT_TRUE(object.is<Value::Object>());
    EXPECT_TRUE(object.as<Value::Object>().at("name").as<bool>());
    EXPECT_TRUE(Value::read_binary(input).empty());
    EXPECT_TRUE(input);
}

TEST(CoreValue, RejectsAShortOrInvalidNestedValueAsAWhole)
{
    Value document{Value::Array{Value{std::string{"abc"}}, Value{true}}};
    std::stringstream valid;
    document.write_binary(valid);
    const auto bytes = valid.str();
    for (std::size_t len = 0; len < bytes.size(); ++len)
    {
        std::istringstream input(bytes.substr(0, len));
        EXPECT_TRUE(Value::read_binary(input).empty());
        EXPECT_TRUE(input.fail());
    }
    for (char byte : {'\x02', '\x7f'})
    {
        auto broken = bytes;
        if (byte == '\x02')
        {
            broken.back() = byte;
        }
        else
        {
            broken[broken.size() - 2] = byte;
        }
        std::istringstream input(broken);
        EXPECT_TRUE(Value::read_binary(input).empty());
        EXPECT_TRUE(input.fail());
    }
}

TEST(CoreValue, ValidatesLengthsBeforeWritingAndHonorsStreamExceptions)
{
    const auto limit = std::numeric_limits<Value::Size>::max();
    Value large{std::string(static_cast<std::size_t>(limit) + 1, 'x')};
    std::ostringstream output;
    output << "prefix";
    Value{Value::Array{large}}.write_binary(output);
    EXPECT_TRUE(output.fail());
    EXPECT_EQ(output.str(), "prefix");

    output.clear();
    output.exceptions(std::ios::failbit);
    EXPECT_THROW(large.write_binary(output), std::ios_base::failure);
    std::istringstream truncated;
    truncated.exceptions(std::ios::failbit);
    EXPECT_THROW(Value::read_binary(truncated), std::ios_base::failure);
}
