#pragma once
#ifndef BENCHMARK_COMMON_HPP__VWYMOSRJ
#define BENCHMARK_COMMON_HPP__VWYMOSRJ

#include <functional>
#include <string>
#include <vector>

namespace {

const auto large_string_list = std::invoke([]() {
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

} //

#endif /* include guard */
