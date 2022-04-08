#include <benchmark/benchmark.h>

#include "jsonwriter/writer.hpp"
#include "benchmark_common.hpp"

int main(int argc, char* argv[])
{
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}

struct SmallStaticStruct {};
struct SmallStruct {};

namespace jsonwriter {

template<>
struct Formatter<SmallStaticStruct> {
#ifndef _MSC_VER
    __attribute__((always_inline)) inline
#endif
    static void write(jsonwriter::Buffer& output, const SmallStaticStruct)
    {
        jsonwriter::write(output, jsonwriter::Object{[](auto& object) {
            object["k1"] = "cd";
            object["k2"] = jsonwriter::Object{[](auto& nested_object) {
                nested_object["o1"] = {1, 2, 999999999};
                nested_object["o2"] = false;
                nested_object["o\r3"] = "i\no";
                nested_object["o4"] = 'c';
            }};
            object["k3"] = false;
            object["k4"] = 234.345678;
        }});
    }
};

template<>
struct Formatter<SmallStruct> {
    static void write(jsonwriter::Buffer& output, const SmallStruct)
    {
        jsonwriter::write(output, jsonwriter::Object{[](auto& object) {
            object[random_strings[0]] = random_strings[1];
            object[random_strings[2]] = jsonwriter::Object{[](auto& nested_object) {
                nested_object[random_strings[3]] = {1, 2, 999999999};
                nested_object[random_strings[4]] = false;
                nested_object[random_strings[5]] = random_strings[6];
                nested_object[random_strings[7]] = 'c';
            }};
            object[random_strings[8]] = false;
            object[random_strings[9]] = 234.345678;
        }});
    }
};

} // namespace jsonwriter

namespace {

void BM_jsonwriter_simple_small_static_struct(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};

    for (auto _ : state) {
        jsonwriter::write(out, SmallStaticStruct{});
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_simple_small_static_struct);

void BM_jsonwriter_simple_small_static_struct_list(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};
    std::vector<SmallStaticStruct> vec{1000};

    for (auto _ : state) {
        jsonwriter::write(out, vec);
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_simple_small_static_struct_list);

void BM_jsonwriter_simple_small_static_struct_dynamic_list(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};
    std::vector<SmallStaticStruct> vec{1000};

    for (auto _ : state) {
        jsonwriter::write(out, jsonwriter::List([&vec](auto& list) {
                              for (const auto& item : vec) {
                                  list.push_back(item);
                              }
                          }));
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_simple_small_static_struct_dynamic_list);

void BM_jsonwriter_simple_small_struct(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};

    for (auto _ : state) {
        jsonwriter::write(out, SmallStruct{});
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_simple_small_struct);

void BM_jsonwriter_large_strings(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};
    out.reserve(large_string_list.size() * large_string_list[0].size() * 2);

    for (auto _ : state) {
        jsonwriter::write(out, large_string_list);
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_large_strings);

void BM_jsonwriter_large_list_of_ints(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};
    out.reserve(large_int_list.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(out, large_int_list);
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_large_list_of_ints);

void BM_jsonwriter_large_list_of_bools(benchmark::State& state)
{
    jsonwriter::SimpleBuffer out{};
    out.reserve(large_bool_list.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(out, large_bool_list);
        benchmark::DoNotOptimize(out.begin());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_jsonwriter_large_list_of_bools);

} // namespace
