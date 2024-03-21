#pragma once
#ifndef BENCHMARK_COMMON_HPP__VWYMOSRJ
#define BENCHMARK_COMMON_HPP__VWYMOSRJ

#include <functional>
#include <random>
#include <string>
#include <vector>

namespace {

inline const auto large_string_list = std::invoke([]() {
    std::vector<std::string> v{};
    for (int i{0}; i < 1000; ++i) {
        v.emplace_back("dddddddd\t\fddaaaa\r\naaaaaaaoooo\\\\\\oooooooo\"\"ooooiiiiiii"
                       "\"iiiiiiiiiiiiiiigggggggs\fsssssssssssssssssssd");
    }
    return v;
});

inline const auto large_int_list = std::invoke([]() {
    std::vector<int> v{};
    for (int i{0}; i < 10000; ++i) {
        v.push_back(i);
    }
    return v;
});

inline const auto large_bool_list = std::invoke([]() {
    std::vector<bool> v{};
    bool value{false};
    for (int i{0}; i < 10000; ++i) {
        v.push_back(value);
        value = !value;
    }
    return v;
});

inline const auto large_float_list = std::invoke([]() {
    std::vector<float> v{};
    for (int i{0}; i < 10000; ++i) {
        v.push_back(static_cast<float>(i));
    }
    return v;
});

inline const auto large_double_list = std::invoke([]() {
    std::vector<double> v{};
    for (int i{0}; i < 10000; ++i) {
        v.push_back(i);
    }
    return v;
});

inline const auto random_strings = std::invoke([]() {
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    std::mt19937 engine{};
    std::uniform_int_distribution<size_t> gen_len{2, 100};
    std::uniform_int_distribution<size_t> gen_char{0, sizeof(alphanum) - 1};

    const auto gen_random = [&]() {
        const auto len = gen_len(engine);
        std::string tmp_s{};
        tmp_s.reserve(len);

        for (size_t i = 0; i < len; ++i) {
            tmp_s += alphanum[gen_char(engine)];
        }

        return tmp_s;
    };

    std::vector<std::string> v{};
    for (int i{0}; i < 100; ++i) {
        v.push_back(gen_random());
    }
    return v;
});

} //

#endif /* include guard */
