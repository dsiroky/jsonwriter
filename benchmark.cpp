#include <benchmark/benchmark.h>
#include <fmt/format.h>

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

namespace {

void BM_jsonwriter_simple_small_struct(benchmark::State& state)
{
    auto out = fmt::memory_buffer();

    for (auto _ : state) {
        jsonwriter::write(out, [](auto& object) {
            object["k1"] = "cd";
            object["k2"] = [](auto& nested_object) {
                nested_object["o1"] = {1, 2, 999999999};
                nested_object["o2"] = false;
                nested_object["o\r3"] = "i\no";
                nested_object["o4"] = 'c';
            };
            object["k3"] = false;
            object["k4"] = 234.345678;
        });
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_simple_small_struct);

void BM_jsonwriter_large_strings(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_string_list.size() * large_string_list[0].size() * 2);

    for (auto _ : state) {
        jsonwriter::write(out, large_string_list);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_large_strings);

void BM_jsonwriter_large_list_of_ints(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_int_list.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(out, large_int_list);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_jsonwriter_large_list_of_ints);

void BM_jsonwriter_large_list_of_bools(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_bool_list.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(out, large_bool_list);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_jsonwriter_large_list_of_bools);

} // namespace
