#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>

#include "io/document-value.hpp"
#include "io/json.hpp"
#include "io/yaml.hpp"

namespace io::tests
{
    namespace
    {
        template <typename T>
        void expect_numeric_round_trip(T original, Number::Type type)
        {
            auto document = DocumentValue::from_core(::Value{original});
            ASSERT_TRUE(document);
            const auto *number = std::get_if<Number>(&document->m_data);
            ASSERT_NE(number, nullptr);
            EXPECT_EQ(number->m_type, type);

            auto round_trip = document->to_core();
            ASSERT_TRUE(round_trip);
            ASSERT_TRUE(round_trip->is<T>());
            EXPECT_EQ(round_trip->as<T>(), original);
            if constexpr (std::floating_point<T>)
            {
                EXPECT_EQ(std::signbit(round_trip->as<T>()), std::signbit(original));
            }
        }
    }

    TEST(DocumentValue, RecoversEveryCoreNumericType)
    {
        expect_numeric_round_trip(std::numeric_limits<std::int32_t>::min(), Number::Type::Int32);
        expect_numeric_round_trip(std::numeric_limits<std::uint32_t>::max(), Number::Type::UInt32);
        expect_numeric_round_trip(std::numeric_limits<std::int64_t>::min(), Number::Type::Int64);
        expect_numeric_round_trip(std::numeric_limits<std::uint64_t>::max(), Number::Type::UInt64);
        expect_numeric_round_trip(std::numeric_limits<char>::min(), Number::Type::Char);
        expect_numeric_round_trip(std::numeric_limits<unsigned char>::max(), Number::Type::UnsignedChar);
        expect_numeric_round_trip(std::numeric_limits<float>::denorm_min(), Number::Type::Float);
        expect_numeric_round_trip(-0.0f, Number::Type::Float);
        expect_numeric_round_trip(std::numeric_limits<double>::max(), Number::Type::Double);
        expect_numeric_round_trip(-0.0, Number::Type::Double);
    }

    TEST(DocumentValue, RecursivelyPreservesObjectsArraysBooleansAndNull)
    {
        const ::Value original{::Value::Object{
            {"items", ::Value{::Value::Array{::Value{true}, ::Value{false}, ::Value{}, ::Value{std::uint32_t{42}}}}},
            {"name", ::Value{std::string{"sample"}}}}};
        auto document = DocumentValue::from_core(original);
        ASSERT_TRUE(document);
        const auto &object = std::get<DocumentValue::Object>(document->m_data);
        const auto &items = std::get<DocumentValue::Array>(object.at("items").m_data);
        ASSERT_EQ(items.size(), 4);
        EXPECT_EQ(std::get<Number>(items.back().m_data).m_type, Number::Type::UInt32);

        auto converted = document->to_core();
        ASSERT_TRUE(converted);
        const auto &core_object = converted->as<::Value::Object>();
        const auto &core_items = core_object.at("items").as<::Value::Array>();
        ASSERT_EQ(core_items.size(), 4);
        EXPECT_TRUE(core_items[0].as<bool>());
        EXPECT_FALSE(core_items[1].as<bool>());
        EXPECT_TRUE(core_items[2].empty());
        EXPECT_EQ(core_items[3].as<std::uint32_t>(), 42);
        EXPECT_EQ(core_object.at("name").as<std::string>(), "sample");
    }

    TEST(DocumentValue, KeepsTypeHintsOutOfJsonAndYaml)
    {
        auto document = DocumentValue::from_core(::Value{std::uint32_t{42}});
        ASSERT_TRUE(document);
        auto json = write_json(*document);
        auto yaml = write_yaml(*document);
        ASSERT_TRUE(json);
        ASSERT_TRUE(yaml);
        EXPECT_EQ(*json, "42");

        auto json_round_trip = parse_json(*json);
        auto yaml_round_trip = parse_yaml(*yaml);
        ASSERT_TRUE(json_round_trip);
        ASSERT_TRUE(yaml_round_trip);
        EXPECT_EQ(*document, *json_round_trip);
        EXPECT_EQ(*document, *yaml_round_trip);
        EXPECT_EQ(std::get<Number>(json_round_trip->m_data).m_type, Number::Type::Unspecified);
        EXPECT_EQ(std::get<Number>(yaml_round_trip->m_data).m_type, Number::Type::Unspecified);
        auto core_number = yaml_round_trip->to_core();
        ASSERT_TRUE(core_number);
        EXPECT_EQ(core_number->as<std::int64_t>(), 42);
    }

    TEST(DocumentValue, InfersTypesForUntaggedNumericTokens)
    {
        auto parsed = parse_json("[-42,18446744073709551615,1.25e2,true,null]");
        ASSERT_TRUE(parsed);
        auto core = parsed->to_core();
        ASSERT_TRUE(core);
        const auto &values = core->as<::Value::Array>();
        ASSERT_EQ(values.size(), 5);
        EXPECT_EQ(values[0].as<std::int64_t>(), -42);
        EXPECT_EQ(values[1].as<std::uint64_t>(), std::numeric_limits<std::uint64_t>::max());
        EXPECT_EQ(values[2].as<double>(), 125.0);
        EXPECT_TRUE(values[3].as<bool>());
        EXPECT_TRUE(values[4].empty());
    }

    TEST(DocumentValue, RejectsInvalidAndOutOfRangeConversions)
    {
        for (const auto *token : {"18446744073709551616", "-9223372036854775809", "1e9999"})
        {
            auto parsed = parse_json(token);
            ASSERT_TRUE(parsed);
            auto result = parsed->to_core();
            ASSERT_FALSE(result);
            EXPECT_EQ(result.error(), ValueConversionError::NumberOutOfRange);
        }
        auto narrow = Number{"256"}.to<unsigned char>();
        ASSERT_FALSE(narrow);
        EXPECT_EQ(narrow.error(), ValueConversionError::NumberOutOfRange);

        auto invalid = Number{"01"}.to<std::int32_t>();
        ASSERT_FALSE(invalid);
        EXPECT_EQ(invalid.error(), ValueConversionError::InvalidNumber);

        auto non_finite = DocumentValue::from_core(::Value{std::numeric_limits<double>::infinity()});
        ASSERT_FALSE(non_finite);
        EXPECT_EQ(non_finite.error(), ValueConversionError::NonFiniteNumber);
    }

    TEST(DocumentValue, BoundsRecursiveConversion)
    {
        DocumentValue document;
        ::Value core;
        for (std::size_t depth = 0; depth < 256; ++depth)
        {
            DocumentValue::Array document_array;
            document_array.push_back(std::move(document));
            document = DocumentValue{std::move(document_array)};
            ::Value::Array core_array;
            core_array.push_back(std::move(core));
            core = ::Value{std::move(core_array)};
        }
        EXPECT_TRUE(document.to_core());
        EXPECT_TRUE(DocumentValue::from_core(core));

        auto deeper_document = DocumentValue{DocumentValue::Array{document}}.to_core();
        ASSERT_FALSE(deeper_document);
        EXPECT_EQ(deeper_document.error(), ValueConversionError::NestingLimit);
        auto deeper_core = DocumentValue::from_core(::Value{::Value::Array{core}});
        ASSERT_FALSE(deeper_core);
        EXPECT_EQ(deeper_core.error(), ValueConversionError::NestingLimit);
    }
}
