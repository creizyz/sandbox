#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "io/json.hpp"
#include "io/yaml.hpp"

namespace io::tests
{
    static_assert(std::is_same_v<decltype(parse_json({}))::value_type, DocumentValue>);
    static_assert(std::is_same_v<decltype(parse_yaml({}))::value_type, DocumentValue>);

    TEST(Yaml, ParsesBlockAndFlowCollectionsWithComments)
    {
        auto yaml = parse_yaml(R"(%YAML 1.2
---
# Configuration
enabled: TRUE
items:
  - name: first # label
    data: [null, False, 1.25, {}]
  - name: "2nd # literal"
    data: []
missing:
...
)");
        auto json = parse_json(R"({"enabled":true,"items":[{"name":"first","data":[null,false,1.25,{}]},{"name":"2nd # literal","data":[]}],"missing":null})");
        ASSERT_TRUE(yaml);
        ASSERT_TRUE(json);
        EXPECT_EQ(*yaml, *json);
    }

    TEST(Yaml, ResolvesPortableCoreScalars)
    {
        auto parsed = parse_yaml(R"([+001, -00.50, .5, 2., 1e9999, 0xFF, 0o17, ~, yes, on, "true", "", !!str 123, !!int "42", !!float "2.", !!bool "false"])");
        auto expected = parse_json(R"([1,-0.50,0.5,2.0,1e9999,255,15,null,"yes","on","true","","123",42,2.0,false])");
        ASSERT_TRUE(parsed);
        ASSERT_TRUE(expected);
        EXPECT_EQ(*parsed, *expected);
    }

    TEST(Yaml, PreservesBlockScalarContent)
    {
        auto literal = parse_yaml("text: |\n  first\n  second # literal\n");
        ASSERT_TRUE(literal);
        auto encoded = write_json(*literal);
        ASSERT_TRUE(encoded);
        EXPECT_EQ(*encoded, R"({"text":"first\u000asecond # literal\u000a"})");

        auto folded = parse_yaml("text: >-\n  first\n  second\n");
        ASSERT_TRUE(folded);
        EXPECT_EQ(write_json(*folded).value(), R"({"text":"first second"})");
    }

    TEST(Yaml, WritesBlockYamlAndPreservesAmbiguousStrings)
    {
        auto json = parse_json(R"({"enabled":true,"label":"true","null":"null","object":{},"sequence":[null,1.2300e+2,"#comment","a\nb\u0000\u00e9"],"text":"123"})");
        ASSERT_TRUE(json);
        auto yaml = write_yaml(*json);
        ASSERT_TRUE(yaml);
        EXPECT_NE(yaml->find("\"enabled\": true\n"), std::string::npos);
        EXPECT_NE(yaml->find("\"sequence\":\n- null\n"), std::string::npos);
        auto round_trip = parse_yaml(*yaml);
        ASSERT_TRUE(round_trip);
        EXPECT_EQ(*round_trip, *json);
    }

    TEST(Yaml, RejectsUnsupportedShapesAndScalars)
    {
        for (const std::string_view input : {"1: value", "true: value", "? [a, b]\n: c", "key: &ref value",
                                             "*ref", "!custom value", "!!int 1.5", "!!binary aA==",
                                             ".inf", "-.INF", ".NaN", "0x10000000000000000",
                                             "%YAML 1.1\n---\nvalue", "a: 1\n\"a\": 2"})
        {
            auto result = parse_yaml(input);
            EXPECT_FALSE(result) << input;
        }
    }

    TEST(Yaml, ReportsMalformedInputAndRejectsMultipleDocuments)
    {
        auto malformed = parse_yaml("a: [1,\nb: 2\n");
        ASSERT_FALSE(malformed);
        ASSERT_TRUE(malformed.error().m_location);
        EXPECT_GE(malformed.error().m_location->m_line, 2);
        EXPECT_GE(malformed.error().m_location->m_column, 1);
        EXPECT_FALSE(parse_yaml("---\na: 1\n---\na: 2\n"));
        EXPECT_FALSE(parse_yaml("# no document\n"));
        auto empty_document = parse_yaml("---\n...\n");
        ASSERT_TRUE(empty_document);
        EXPECT_EQ(*empty_document, DocumentValue{});
    }

    TEST(Yaml, EnforcesTheNestingLimit)
    {
        const auto nested = std::string(256, '[') + "null" + std::string(256, ']');
        auto value = parse_yaml(nested);
        ASSERT_TRUE(value);
        EXPECT_TRUE(write_yaml(*value));
        EXPECT_FALSE(parse_yaml("[" + nested + "]"));
        EXPECT_FALSE(write_yaml(DocumentValue{DocumentValue::Array{*value}}));
    }

    TEST(Yaml, ReaderErrorsCountCharactersFromTheStartOfTheStream)
    {
        auto utf8 = parse_yaml("\xef\xbb\xbf# header\r\nkey: \xc3\xa9\xff");
        ASSERT_FALSE(utf8);
        ASSERT_TRUE(utf8.error().m_location);
        EXPECT_EQ(utf8.error().m_location->m_line, 2);
        EXPECT_EQ(utf8.error().m_location->m_column, 7);

        // UTF-16LE BOM, a newline, one supplementary character, then an incomplete unit.
        constexpr char utf16[] = "\xff\xfe#\0\n\0\x3d\xd8\x00\xde\xff";
        auto result = parse_yaml(std::string_view{utf16, sizeof(utf16) - 1});
        ASSERT_FALSE(result);
        ASSERT_TRUE(result.error().m_location);
        EXPECT_EQ(result.error().m_location->m_line, 2);
        EXPECT_EQ(result.error().m_location->m_column, 2);
    }

    TEST(Yaml, HandlesStreamsAndRejectsInvalidOutput)
    {
        std::istringstream input{"# configuration\nname: example\n"};
        auto value = read_yaml(input);
        ASSERT_TRUE(value);
        std::ostringstream output;
        EXPECT_TRUE(write_yaml(output, *value));
        EXPECT_EQ(output.str(), "\"name\": \"example\"\n");

        EXPECT_FALSE(write_yaml(output, DocumentValue{Number{"01"}}));
        EXPECT_FALSE(write_yaml(output, DocumentValue{std::string{"\xed\xa0\x80"}}));
        EXPECT_EQ(output.str(), "\"name\": \"example\"\n");

        input.setstate(std::ios::badbit);
        output.setstate(std::ios::badbit);
        EXPECT_FALSE(read_yaml(input));
        EXPECT_FALSE(write_yaml(output, *value));
    }
}
