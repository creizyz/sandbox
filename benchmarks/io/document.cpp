#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "core/value.hpp"
#include "io/document-value.hpp"
#include "io/json.hpp"
#include "io/yaml.hpp"

namespace
{
    Value make_core_document(std::int64_t record_count)
    {
        Value::Array records;
        records.reserve(static_cast<std::size_t>(record_count));
        for (std::int64_t index = 0; index < record_count; ++index)
        {
            records.emplace_back(Value::Object{
                {"id", Value{static_cast<std::uint32_t>(index)}},
                {"enabled", Value{index % 2 == 0}},
                {"name", Value{"service-" + std::to_string(index)}},
                {"limits", Value{Value::Object{
                               {"max_connections", Value{100 + index}},
                               {"retry_seconds", Value{1.25f + static_cast<float>(index % 8) * 0.125f}},
                           }}},
                {"tags", Value{Value::Array{Value{std::string{"internal"}}, Value{"tier-" + std::to_string(index % 4)}}}},
            });
        }
        return Value{Value::Object{
            {"description", Value{}},
            {"records", Value{std::move(records)}},
        }};
    }

    struct JsonCodec
    {
        static auto read(std::string_view text)
        {
            return io::parse_json(text);
        }
        static auto write(const io::DocumentValue &value)
        {
            return io::write_json(value);
        }
    };

    struct YamlCodec
    {
        static auto read(std::string_view text)
        {
            return io::parse_yaml(text);
        }
        static auto write(const io::DocumentValue &value)
        {
            return io::write_yaml(value);
        }
    };

    template <typename Codec, bool Parsing>
    void benchmark_codec(benchmark::State &state)
    {
        const auto record_count = state.range(0);
        const auto core = make_core_document(record_count);
        const auto document = io::DocumentValue::from_core(core);
        if (!document)
        {
            state.SkipWithError("could not convert the fixture to a document");
            return;
        }
        const auto encoded = Codec::write(*document);
        if (!encoded)
        {
            state.SkipWithError("could not encode the document fixture");
            return;
        }
        const auto decoded = Codec::read(*encoded);
        if (!decoded || *decoded != *document)
        {
            state.SkipWithError("document fixture did not round-trip");
            return;
        }

        for (auto _ : state)
        {
            if constexpr (Parsing)
            {
                auto result = Codec::read(*encoded);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = Codec::write(*document);
                benchmark::DoNotOptimize(result);
            }
        }

        const auto bytes = static_cast<std::int64_t>(encoded->size());
        state.counters["document_bytes"] = static_cast<double>(bytes);
        state.SetBytesProcessed(state.iterations() * bytes);
        state.SetItemsProcessed(state.iterations() * record_count);
    }

    template <bool FromCore>
    void benchmark_conversion(benchmark::State &state)
    {
        const auto record_count = state.range(0);
        const auto core = make_core_document(record_count);
        const auto document = io::DocumentValue::from_core(core);
        if (!document)
        {
            state.SkipWithError("could not convert the fixture to a document");
            return;
        }
        const auto converted = document->to_core();
        if (!converted || io::DocumentValue::from_core(*converted) != document)
        {
            state.SkipWithError("core value fixture did not round-trip");
            return;
        }

        for (auto _ : state)
        {
            if constexpr (FromCore)
            {
                auto result = io::DocumentValue::from_core(core);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = document->to_core();
                benchmark::DoNotOptimize(result);
            }
        }
        state.SetItemsProcessed(state.iterations() * record_count);
    }

    void document_sizes(benchmark::Benchmark *entry)
    {
        entry->Arg(16)->Arg(256)->Arg(4096)->ArgName("records")->Unit(benchmark::kMicrosecond);
    }

    BENCHMARK(benchmark_codec<JsonCodec, true>)->Name("bm_document_parse_json")->Apply(document_sizes);
    BENCHMARK(benchmark_codec<JsonCodec, false>)->Name("bm_document_write_json")->Apply(document_sizes);
    BENCHMARK(benchmark_codec<YamlCodec, true>)->Name("bm_document_parse_yaml")->Apply(document_sizes);
    BENCHMARK(benchmark_codec<YamlCodec, false>)->Name("bm_document_write_yaml")->Apply(document_sizes);
    BENCHMARK(benchmark_conversion<true>)->Name("bm_document_from_core")->Apply(document_sizes);
    BENCHMARK(benchmark_conversion<false>)->Name("bm_document_to_core")->Apply(document_sizes);
}
