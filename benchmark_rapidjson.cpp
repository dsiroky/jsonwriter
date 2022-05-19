#include <benchmark/benchmark.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "3rdparty/rapidjson/include/rapidjson/document.h"
#include "3rdparty/rapidjson/include/rapidjson/writer.h"
#include "3rdparty/rapidjson/include/rapidjson/stringbuffer.h"

#include "benchmark_common.hpp"

namespace {

void BM_rapidjson_simple_small_static_struct(benchmark::State& state)
{
    namespace rj = rapidjson;

    for (auto _ : state) {
        rj::Document d{};
        d.SetObject();
        d.AddMember("k1", "cd", d.GetAllocator());
        rj::Value o2{rj::kObjectType};
        {
            rj::Value a{rj::kArrayType};
            a.PushBack(1, d.GetAllocator());
            a.PushBack(2, d.GetAllocator());
            a.PushBack(999999999, d.GetAllocator());
            o2.AddMember("o1", a, d.GetAllocator());
            o2.AddMember("o2", false, d.GetAllocator());
            o2.AddMember("o\r3", "i\no", d.GetAllocator());
            o2.AddMember("o4", 'c', d.GetAllocator());
        }
        d.AddMember("k2", o2.Move(), d.GetAllocator());
        d.AddMember("k3", false, d.GetAllocator());
        d.AddMember("k4", 234.345678, d.GetAllocator());
        rj::StringBuffer buffer;
        rj::Writer<rj::StringBuffer> writer(buffer);
        d.Accept(writer);

        benchmark::DoNotOptimize(buffer.GetString());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_rapidjson_simple_small_static_struct);

void BM_rapidjson_simple_small_static_struct_dump_only(benchmark::State& state)
{
    namespace rj = rapidjson;

    rj::Document d{};
    d.SetObject();
    d.AddMember("k1", "cd", d.GetAllocator());
    rj::Value o2{rj::kObjectType};
    {
        rj::Value a{rj::kArrayType};
        a.PushBack(1, d.GetAllocator());
        a.PushBack(2, d.GetAllocator());
        a.PushBack(999999999, d.GetAllocator());
        o2.AddMember("o1", a, d.GetAllocator());
        o2.AddMember("o2", false, d.GetAllocator());
        o2.AddMember("o\r3", "i\no", d.GetAllocator());
        o2.AddMember("o4", 'c', d.GetAllocator());
    }
    d.AddMember("k2", o2.Move(), d.GetAllocator());
    d.AddMember("k3", false, d.GetAllocator());
    d.AddMember("k4", 234.345678, d.GetAllocator());

    for (auto _ : state) {
        rj::StringBuffer buffer;
        rj::Writer<rj::StringBuffer> writer(buffer);
        d.Accept(writer);

        benchmark::DoNotOptimize(buffer.GetString());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_rapidjson_simple_small_static_struct_dump_only);

void BM_rapidjson_simple_small_static_struct_list(benchmark::State& state)
{
    namespace rj = rapidjson;

    for (auto _ : state) {
        rj::Document d{};
        d.SetArray();
        for (int i{0}; i < 1000; ++i) {
            rj::Value item{rj::kObjectType};
            item.AddMember("k1", "cd", d.GetAllocator());
            rj::Value o2{rj::kObjectType};
            {
                rj::Value a{rj::kArrayType};
                a.PushBack(1, d.GetAllocator());
                a.PushBack(2, d.GetAllocator());
                a.PushBack(999999999, d.GetAllocator());
                o2.AddMember("o1", a, d.GetAllocator());
                o2.AddMember("o2", false, d.GetAllocator());
                o2.AddMember("o\r3", "i\no", d.GetAllocator());
                o2.AddMember("o4", 'c', d.GetAllocator());
            }
            item.AddMember("k2", o2.Move(), d.GetAllocator());
            item.AddMember("k3", false, d.GetAllocator());
            item.AddMember("k4", 234.345678, d.GetAllocator());
            d.PushBack(item, d.GetAllocator());
        }
        rj::StringBuffer buffer;
        rj::Writer<rj::StringBuffer> writer(buffer);
        d.Accept(writer);

        benchmark::DoNotOptimize(buffer.GetString());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_rapidjson_simple_small_static_struct_list);

void BM_rapidjson_simple_small_struct(benchmark::State& state)
{
    namespace rj = rapidjson;

    for (auto _ : state) {
        rj::Document d{};
        d.SetObject();
        d.AddMember(rapidjson::StringRef(random_strings[0]),
                    rapidjson::StringRef(random_strings[1]), d.GetAllocator());
        rj::Value o2{rj::kObjectType};
        {
            rj::Value a{rj::kArrayType};
            a.PushBack(1, d.GetAllocator());
            a.PushBack(2, d.GetAllocator());
            a.PushBack(999999999, d.GetAllocator());
            o2.AddMember(rapidjson::StringRef(random_strings[2]), a, d.GetAllocator());
            o2.AddMember(rapidjson::StringRef(random_strings[3]), false, d.GetAllocator());
            o2.AddMember(rapidjson::StringRef(random_strings[4]),
                         rapidjson::StringRef(random_strings[5]), d.GetAllocator());
            o2.AddMember(rapidjson::StringRef(random_strings[6]), 'c', d.GetAllocator());
        }
        d.AddMember(rapidjson::StringRef(random_strings[7]), o2.Move(), d.GetAllocator());
        d.AddMember(rapidjson::StringRef(random_strings[8]), false, d.GetAllocator());
        d.AddMember(rapidjson::StringRef(random_strings[9]), 234.345678, d.GetAllocator());
        rj::StringBuffer buffer;
        rj::Writer<rj::StringBuffer> writer(buffer);
        d.Accept(writer);

        benchmark::DoNotOptimize(buffer.GetString());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_rapidjson_simple_small_struct);

void BM_rapidjson_large_strings(benchmark::State& state)
{
    namespace rj = rapidjson;

    for (auto _ : state) {
        rj::Document d{};
        d.SetArray();
        for (const auto& s: large_string_list) {
            rj::Value value{};
            value.SetString(s.c_str(), static_cast<rj::SizeType>(s.size()), d.GetAllocator());
            d.PushBack(value, d.GetAllocator());
        }
        rj::StringBuffer buffer;
        rj::Writer<rj::StringBuffer> writer(buffer);
        d.Accept(writer);

        benchmark::DoNotOptimize(buffer.GetString());
        benchmark::DoNotOptimize(buffer.GetLength());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_rapidjson_large_strings);

void BM_rapidjson_large_strings_dump_only(benchmark::State& state)
{
    namespace rj = rapidjson;

    rj::Document d{};
    d.SetArray();
    for (const auto& s: large_string_list) {
        rj::Value value{};
        value.SetString(s.c_str(), static_cast<rj::SizeType>(s.size()), d.GetAllocator());
        d.PushBack(value, d.GetAllocator());
    }

    for (auto _ : state) {
        rj::StringBuffer buffer;
        rj::Writer<rj::StringBuffer> writer(buffer);
        d.Accept(writer);

        benchmark::DoNotOptimize(buffer.GetString());
        benchmark::DoNotOptimize(buffer.GetLength());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_rapidjson_large_strings_dump_only);

} // namespace
