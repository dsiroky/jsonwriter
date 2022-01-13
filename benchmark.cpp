#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <boost/container/static_vector.hpp>

#include "jsonwriter/writer.hpp"

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

//--------------------------------------------------------------------------

const auto large_vector = std::invoke([]() {
    std::vector<std::string> v{};
    for (int i{0}; i < 1000; ++i) {
        v.emplace_back("dddddddd\t\fddaaaa\r\naaaaaaaoooo\\\\\\oooooooo\"\"ooooiiiiiii"
                       "\"iiiiiiiiiiiiiiigggggggs\fsssssssssssssssssssd");
    }
    return v;
});

const auto large_int_list = std::invoke([]() {
    std::vector<int> v{};
    for (int i{0}; i < 10000; ++i) {
        v.push_back(i);
    }
    return v;
});

const auto large_bool_list = std::invoke([]() {
    std::vector<bool> v{};
    bool value{false};
    for (int i{0}; i < 10000; ++i) {
        v.push_back(value);
        value = !value;
    }
    return v;
});

//--------------------------------------------------------------------------

void BM_json_simple_small_struct(benchmark::State& state)
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
BENCHMARK(BM_json_simple_small_struct);

void BM_json_large_strings(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_vector.size() * large_vector[0].size() * 2);

    for (auto _ : state) {
        jsonwriter::write(out, large_vector);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_json_large_strings);

void BM_json_large_list_of_ints(benchmark::State& state)
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
BENCHMARK(BM_json_large_list_of_ints);

void BM_json_large_list_of_bools(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_bool_list.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(out, large_bool_list);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_json_large_list_of_bools);

} // namespace
