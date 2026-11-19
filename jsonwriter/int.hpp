#pragma once
#ifndef INT_HPP__2WHLC6OG
#define INT_HPP__2WHLC6OG

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace jsonwriter {

// Conversion helpers are called once per value, so an out-of-line call would
// dominate the cost of formatting small integers. Force them into the caller.
#if defined(_MSC_VER)
#define JSONWRITER_INT_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define JSONWRITER_INT_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define JSONWRITER_INT_ALWAYS_INLINE inline
#endif

class FormatInt
{
public:
    template<typename T>
    std::string_view itostr(const T val)
    {
        static_assert(std::is_integral_v<T>);
        return {buf, static_cast<size_t>(itostr_to(buf, val) - buf)};
    }

    /// Writes the decimal representation of val to out and returns one past the
    /// last written character. The caller must provide space for the longest
    /// value: 20 bytes.
    template<typename T>
    static JSONWRITER_INT_ALWAYS_INLINE char* itostr_to(char* out, const T val) noexcept
    {
        static_assert(std::is_integral_v<T>);
        if constexpr (std::is_signed_v<T>) {
            if (val < 0) {
                *out++ = '-';
                // Negate as unsigned: the minimum value overflows otherwise.
                return write_uint(out, uint64_t{0} - static_cast<uint64_t>(val));
            }
        }
        return write_uint(out, static_cast<uint64_t>(val));
    }

private:
    // taken from https://stackoverflow.com/questions/4351371/c-performance-challenge-integer-to-stdstring-conversion

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

    static JSONWRITER_INT_ALWAYS_INLINE unsigned count_digits(uint64_t val) noexcept
    {
        unsigned count{1};
        while (val >= 10000) {
            val /= 10000;
            count += 4;
        }
        if (val >= 1000) {
            return count + 3;
        }
        if (val >= 100) {
            return count + 2;
        }
        if (val >= 10) {
            return count + 1;
        }
        return count;
    }

    /// Writes val from its end towards out, so the length must be known first.
    static JSONWRITER_INT_ALWAYS_INLINE char* write_uint(char* const out, uint64_t val) noexcept
    {
        const unsigned length = count_digits(val);
        char* it = out + length;
        while (val >= 100) {
            const auto pair = static_cast<unsigned>(val % 100);
            val /= 100;
            it -= 2;
            std::memcpy(it, &digit_pairs[2 * pair], 2);
        }
        if (val >= 10) {
            it -= 2;
            std::memcpy(it, &digit_pairs[2 * val], 2);
        } else {
            *--it = static_cast<char>('0' + val);
        }
        return out + length;
    }

    // fit int64_t
    static constexpr int BUFFER_SIZE{20};
    char buf[BUFFER_SIZE];
};

} // namespace jsonwriter

#undef JSONWRITER_INT_ALWAYS_INLINE

#endif /* include guard */
