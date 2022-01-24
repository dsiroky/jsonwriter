#pragma once
#ifndef INT_HPP__2WHLC6OG
#define INT_HPP__2WHLC6OG

#include <cstring>
#include <string_view>
#include <type_traits>

namespace jsonwriter {

class FormatInt
{
    static constexpr char digit_pairs[201] = {"00010203040506070809"
                                              "10111213141516171819"
                                              "20212223242526272829"
                                              "30313233343536373839"
                                              "40414243444546474849"
                                              "50515253545556575859"
                                              "60616263646566676869"
                                              "70717273747576777879"
                                              "80818283848586878889"
                                              "90919293949596979899"};

    // fit int64_t
    static const int BUFFER_SIZE = 20;
    char buf[BUFFER_SIZE];

public:
    template<typename T>
    std::string_view itostr(const T val)
    {
        static_assert(std::is_integral_v<T>);
        if constexpr (std::is_signed_v<T>) {
            return itostr_int(val);
        } else {
            return itostr_uint(val);
        }
    }

private:
    // taken from https://stackoverflow.com/questions/4351371/c-performance-challenge-integer-to-stdstring-conversion

    std::string_view itostr_int(int64_t val)
    {
        char* it = &buf[BUFFER_SIZE - 2];

        if (val >= 0) {
            int64_t div = val / 100;
            while (div) {
                memcpy(it, &digit_pairs[2 * (val - div * 100)], 2);
                val = div;
                it -= 2;
                div = val / 100;
            }
            memcpy(it, &digit_pairs[2 * val], 2);
            if (val < 10)
                it++;
        } else {
            int64_t div = val / 100;
            while (div) {
                memcpy(it, &digit_pairs[-2 * (val - div * 100)], 2);
                val = div;
                it -= 2;
                div = val / 100;
            }
            memcpy(it, &digit_pairs[-2 * val], 2);
            if (val <= -10)
                it--;
            *it = '-';
        }

        return std::string_view(it, static_cast<size_t>(&buf[BUFFER_SIZE] - it));
    }

    std::string_view itostr_uint(uint64_t val)
    {
        char* it = &buf[BUFFER_SIZE - 2];

        uint64_t div = val / 100;
        while (div) {
            memcpy(it, &digit_pairs[2 * (val - div * 100)], 2);
            val = div;
            it -= 2;
            div = val / 100;
        }
        memcpy(it, &digit_pairs[2 * val], 2);
        if (val < 10)
            it++;

        return std::string_view(it, static_cast<size_t>(&buf[BUFFER_SIZE] - it));
    }
};

} // namespace jsonwriter

#endif /* include guard */
