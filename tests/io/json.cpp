#include <gtest/gtest.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "io/json.hpp"

namespace io::tests
{
    TEST(Json, PreservesOrdinaryObjectsAndAllValueKinds)
    {
        constexpr auto input = R"({"items":[null,true,false,123.25,"text",[],{}],"type":"int64","value":42})";
        auto parsed = parse_json(input);
        ASSERT_TRUE(parsed);
        const auto &object = std::get<JsonValue::Object>(parsed->m_data);
        const auto &array = std::get<JsonValue::Array>(object.at("items").m_data);
        ASSERT_EQ(array.size(), 7);
        EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(array[0].m_data));
        EXPECT_TRUE(std::get<bool>(array[1].m_data));
        EXPECT_FALSE(std::get<bool>(array[2].m_data));
        EXPECT_EQ(std::get<JsonNumber>(array[3].m_data).m_text, "123.25");
        EXPECT_EQ(std::get<std::string>(array[4].m_data), "text");
        EXPECT_TRUE(std::get<JsonValue::Array>(array[5].m_data).empty());
        EXPECT_TRUE(std::get<JsonValue::Object>(array[6].m_data).empty());
        EXPECT_EQ(std::get<std::string>(object.at("type").m_data), "int64");
        EXPECT_EQ(std::get<JsonNumber>(object.at("value").m_data).m_text, "42");

        auto compact = write_json(*parsed);
        ASSERT_TRUE(compact);
        EXPECT_EQ(*compact, input);
        auto pretty = write_json(*parsed, true);
        ASSERT_TRUE(pretty);
        EXPECT_NE(pretty->find("\n  \"items\": ["), std::string::npos);
        auto round_trip = parse_json(*pretty);
        ASSERT_TRUE(round_trip);
        EXPECT_EQ(*round_trip, *parsed);
    }

    TEST(Json, AcceptsCommentsBetweenTokens)
    {
        constexpr auto input = R"(
            /* before */ {
                "items" /* key */ : [1 /* after value */, // after comma
                    null, {"text": "// literal /* text */"}]
                // before close
            } /* after */ // at EOF)";
        auto parsed = parse_json(input);
        ASSERT_TRUE(parsed);
        auto text = write_json(*parsed);
        ASSERT_TRUE(text);
        EXPECT_EQ(*text, R"({"items":[1,null,{"text":"// literal /* text */"}]})");
        auto trailing = parse_json("true// end\rfalse");
        ASSERT_FALSE(trailing);
        EXPECT_EQ(trailing.error().m_offset, 11);
    }

    TEST(Json, ReportsUnterminatedCommentsAtTheirStart)
    {
        auto parsed = parse_json("null /* unfinished");
        ASSERT_FALSE(parsed);
        EXPECT_EQ(parsed.error().m_offset, 5);
        EXPECT_EQ(parsed.error().m_message, "unterminated block comment");
    }

    TEST(Json, RejectsMalformedDocuments)
    {
        for (const std::string_view input : {"", "/* only */", "tru", "true false", "nullx", "1/* split */2",
                                             "[1,]", "[,1]", "[1 2]", "[1,,]", "{]", R"({"a":1,})",
                                             R"({"a" 1})", "-01", ".1", "1.", "+1", "1e", "1e+",
                                             "\"unescaped\n\"", R"("\q")", R"("\u12")",
                                             R"("\ud800")", R"("\udc00")", R"("\ud800\u0041")"})
        {
            auto parsed = parse_json(input);
            EXPECT_FALSE(parsed) << input;
            if (!parsed)
            {
                EXPECT_LE(parsed.error().m_offset.value(), input.size());
            }
        }
    }

    TEST(Json, DecodesUnicodeAndEscapesObjectKeysAndStrings)
    {
        auto parsed = parse_json(R"({"quote\"":"\u00e9\u20ac\uD83D\uDE00\u0000\n\/\"\\\b\f\r\t"})");
        ASSERT_TRUE(parsed);
        const auto &object = std::get<JsonValue::Object>(parsed->m_data);
        const auto &text = std::get<std::string>(object.at("quote\"").m_data);
        const char expected[] = "\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80\0\n/\"\\\b\f\r\t";
        EXPECT_EQ(text, std::string(expected, sizeof(expected) - 1));
        auto encoded = write_json(*parsed);
        ASSERT_TRUE(encoded);
        EXPECT_NE(encoded->find(R"(\u0000\u000a)"), std::string::npos);
        auto round_trip = parse_json(*encoded);
        ASSERT_TRUE(round_trip);
        EXPECT_EQ(*round_trip, *parsed);
    }

    TEST(Json, RejectsInvalidRawUtf8OnReadAndWrite)
    {
        for (const std::string_view bytes : {"\x80", "\xc0\x80", "\xe2\x82", "\xed\xa0\x80", "\xf4\x90\x80\x80"})
        {
            EXPECT_FALSE(parse_json(std::string{"\""} + std::string(bytes) + "\""));
            EXPECT_FALSE(write_json(JsonValue{std::string(bytes)}));
        }
    }

    TEST(Json, PreservesNumberTextWithoutConversion)
    {
        for (const std::string_view number : {"-0", "-1.2300E+42", "1e9999", "18446744073709551616"})
        {
            auto parsed = parse_json(number);
            ASSERT_TRUE(parsed);
            EXPECT_EQ(std::get<JsonNumber>(parsed->m_data).m_text, number);
            EXPECT_EQ(write_json(*parsed).value(), number);
        }
        EXPECT_FALSE(write_json(JsonValue{JsonNumber{"NaN"}}));
        EXPECT_FALSE(write_json(JsonValue{JsonNumber{"01"}}));
    }

    TEST(Json, UsesTheLastDuplicateMember)
    {
        auto parsed = parse_json(R"({"a":1,"\u0061":2})");
        ASSERT_TRUE(parsed);
        EXPECT_EQ(write_json(*parsed).value(), R"({"a":2})");
    }

    TEST(Json, EnforcesTheSameNestingLimitOnReadAndWrite)
    {
        const auto nested = std::string(256, '[') + "null" + std::string(256, ']');
        auto parsed = parse_json(nested);
        ASSERT_TRUE(parsed);
        EXPECT_EQ(write_json(*parsed).value(), nested);
        EXPECT_FALSE(parse_json("[" + nested + "]"));
        EXPECT_FALSE(write_json(JsonValue{JsonValue::Array{std::move(*parsed)}}));
    }

    TEST(Json, ReadsAndWritesStreams)
    {
        std::istringstream input{"/* comment */ null"};
        auto parsed = read_json(input);
        ASSERT_TRUE(parsed);
        EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(parsed->m_data));

        std::ostringstream output;
        output << std::setw(10) << std::setfill('*');
        EXPECT_TRUE(write_json(output, *parsed));
        EXPECT_EQ(output.str(), "null");

        output.setstate(std::ios::badbit);
        EXPECT_FALSE(write_json(output, *parsed));
        input.setstate(std::ios::badbit);
        EXPECT_FALSE(read_json(input));
    }

    TEST(Json, KeepsTheStreamUnchangedIfTheValueIsInvalid)
    {
        std::ostringstream output;
        output << "prefix";
        auto error = write_json(output, JsonValue{JsonValue::Array{JsonValue{}, JsonValue{JsonNumber{"1x"}}}});
        EXPECT_FALSE(error);
        EXPECT_EQ(output.str(), "prefix");
    }
}
