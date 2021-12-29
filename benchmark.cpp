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

constexpr std::array<int, 10000> large_int_list{};

const auto large_bool_vector = std::invoke([]() {
    std::vector<bool> v{};
    bool value{false};
    for (int i{0}; i < 20000; ++i) {
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
        jsonwriter::write(std::back_inserter(out), [](auto& object) {
            object["k1"] = "cd";
            object["k2"] = [](auto& nested_object) {
                nested_object["o1"] = {1, 2, 999999999};
                nested_object["o2"] = false;
                nested_object["o\r3"] = "i\no";
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

void BM_json_simple_small_struct_no_backinsert(benchmark::State& state)
{
    std::array<char, 500> out;

    for (auto _ : state) {
        jsonwriter::write(out.begin(), [](auto& object) {
            object["k1"] = "cd";
            object["k2"] = [](auto& nested_object) {
                nested_object["o1"] = {1, 2, 999999999};
                nested_object["o2"] = false;
                nested_object["o\r3"] = "i\no";
            };
            object["k3"] = false;
            object["k4"] = 234.345678;
        });
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_json_simple_small_struct_no_backinsert);

void BM_json_large_strings(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_vector.size() * large_vector[0].size() * 2);

    for (auto _ : state) {
        jsonwriter::write(std::back_inserter(out), large_vector);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_json_large_strings);

void BM_json_large_strings_no_backinsert(benchmark::State& state)
{
    std::array<char, 300000> out;

    for (auto _ : state) {
        jsonwriter::write(out.begin(), large_vector);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_json_large_strings_no_backinsert);

void BM_json_large_list_of_ints(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_int_list.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(std::back_inserter(out), large_int_list);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_json_large_list_of_ints);

void BM_json_large_list_of_ints_no_backinsert(benchmark::State& state)
{
    std::array<char, large_int_list.size() * 10> out;

    for (auto _ : state) {
        jsonwriter::write(out.begin(), large_int_list);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_json_large_list_of_ints_no_backinsert);

void BM_json_large_list_of_bools_no_backinsert(benchmark::State& state)
{
    std::vector<char> out(large_bool_vector.size() * 10);

    for (auto _ : state) {
        jsonwriter::write(out.begin(), large_bool_vector);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_json_large_list_of_bools_no_backinsert);

void BM_backinsert_large_strings(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.reserve(large_vector.size() * large_vector[0].size());

    for (auto _ : state) {
        auto out_it = std::back_inserter(out);
        for (const auto& item: large_vector) {
            out_it = std::copy(item.begin(), item.end(), out_it);
        }
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
        out.clear();
    }
}
BENCHMARK(BM_backinsert_large_strings);

void BM_copy_large_strings_no_backinsert(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.resize(large_vector.size() * large_vector[0].size());

    for (auto _ : state) {
        auto out_it = out.begin();
        for (const auto& item: large_vector) {
            out_it = std::copy(item.begin(), item.end(), out_it);
        }
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_copy_large_strings_no_backinsert);

void BM_copy_if_large_strings_no_backinsert(benchmark::State& state)
{
    auto out = fmt::memory_buffer();
    out.resize(large_vector.size() * large_vector[0].size());

    for (auto _ : state) {
        auto out_it = out.begin();
        for (const auto& item: large_vector) {
            out_it = std::copy_if(item.begin(), item.end(), out_it, [](const char c){return c >= ' ';});
        }
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_copy_if_large_strings_no_backinsert);

} // namespace
