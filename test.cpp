#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "jsonwriter/writer.hpp"

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

static std::string to_str(const jsonwriter::Buffer& b) { return std::string{b.begin(), b.end()}; }

static std::vector<char> long_data{std::invoke([]() {
    std::vector<char> v(10000);
    for (size_t i{0}; i < v.size(); ++i) {
        v[i] = static_cast<char>(i);
    }
    return v;
})};

static_assert(std::is_nothrow_move_constructible_v<jsonwriter::SimpleBuffer<>>);
static_assert(std::is_nothrow_move_assignable_v<jsonwriter::SimpleBuffer<>>);

//==========================================================================

TEST(TestJsonBuffer, GrowConsume)
{
    jsonwriter::SimpleBuffer out{};

    out.make_room(10);
    EXPECT_GE(out.room(), 10u);
    EXPECT_GE(out.capacity(), 10u);
    out.consume(std::copy_n(long_data.begin(), 10, out.working_end()));
    EXPECT_EQ((std::string_view{out.data(), 10}), (std::string_view{long_data.data(), 10}));

    out.make_room(100);
    EXPECT_GE(out.room(), 100u);
    EXPECT_GE(out.capacity(), 110u);
    EXPECT_EQ((std::string_view{out.data(), 10}), (std::string_view{long_data.data(), 10}));

    out.make_room(1000);
    EXPECT_GE(out.room(), 1000u);
    EXPECT_GE(out.capacity(), 1010u);
    EXPECT_EQ((std::string_view{out.data(), 10}), (std::string_view{long_data.data(), 10}));

    out.consume(std::copy_n(long_data.begin() + 10, 900 - 10, out.working_end()));
    EXPECT_EQ((std::string_view{out.data(), 900}), (std::string_view{long_data.data(), 900}));

    out.make_room(10000);
    EXPECT_GE(out.room(), 10000u);
    EXPECT_GE(out.capacity(), 10900u);
    EXPECT_EQ((std::string_view{out.data(), 900}), (std::string_view{long_data.data(), 900}));
}

TEST(TestJsonBuffer, Move)
{
    {
        jsonwriter::SimpleBuffer orig{};
        orig.make_room(10);
        orig.consume(std::copy_n(long_data.begin(), 10, orig.working_end()));

        {
            jsonwriter::SimpleBuffer newbuf{std::move(orig)};
            EXPECT_EQ(newbuf.size(), 10u);
            EXPECT_EQ((std::string_view{newbuf.data(), 10}),
                      (std::string_view{long_data.data(), 10}));

            // this should be undefined, nullptr test is a white box test
            EXPECT_TRUE(orig.begin() == nullptr);
        }
    }

    {
        jsonwriter::SimpleBuffer orig{};
        orig.make_room(10);
        orig.consume(std::copy_n(long_data.begin(), 10, orig.working_end()));

        {
            jsonwriter::SimpleBuffer newbuf{};
            newbuf = std::move(orig);
            EXPECT_EQ(newbuf.size(), 10u);
            EXPECT_EQ((std::string_view{newbuf.data(), 10}),
                      (std::string_view{long_data.data(), 10}));

            // this should be undefined, nullptr test is a white box test
            EXPECT_TRUE(orig.begin() == nullptr);
        }
    }
}

TEST(TestJsonBuffer, MoveDynamic)
{
    const auto data = std::string_view{long_data.data(), long_data.size()};

    // move construction transfers the dynamic storage
    {
        jsonwriter::SimpleBuffer orig{};
        orig.make_room(data.size());
        orig.consume(std::copy_n(data.begin(), data.size(), orig.working_end()));

        jsonwriter::SimpleBuffer newbuf{std::move(orig)};
        EXPECT_EQ((std::string_view{newbuf.data(), newbuf.size()}), data);

        // this should be undefined, nullptr test is a white box test
        EXPECT_TRUE(orig.begin() == nullptr);
    }

    // move assignment of a static buffer into a grown buffer
    {
        jsonwriter::SimpleBuffer orig{};
        orig.make_room(10);
        orig.consume(std::copy_n(long_data.begin(), 10, orig.working_end()));

        jsonwriter::SimpleBuffer newbuf{};
        newbuf.make_room(data.size());
        newbuf.consume(std::copy_n(data.begin(), data.size(), newbuf.working_end()));

        newbuf = std::move(orig);
        EXPECT_EQ((std::string_view{newbuf.data(), newbuf.size()}),
                  (std::string_view{long_data.data(), 10}));

        // this should be undefined, nullptr test is a white box test
        EXPECT_TRUE(orig.begin() == nullptr);

        // the destination must still be growable after the move
        newbuf.make_room(data.size());
        newbuf.consume(std::copy_n(data.begin(), data.size(), newbuf.working_end()));
        EXPECT_EQ(newbuf.size(), 10 + data.size());
        EXPECT_EQ((std::string_view{newbuf.data() + 10, data.size()}), data);
    }
}

TEST(TestJsonBuffer, MoveConstructionPreservesContext)
{
    jsonwriter::SimpleBuffer orig{};
    orig.context = std::string{"hello"};

    jsonwriter::SimpleBuffer newbuf{std::move(orig)};

    ASSERT_TRUE(newbuf.context.has_value());
    EXPECT_EQ(std::any_cast<std::string>(newbuf.context), "hello");
}

TEST(TestJsonBuffer, MoveAssignmentPreservesContext)
{
    jsonwriter::SimpleBuffer orig{};
    orig.context = std::string{"hello"};
    jsonwriter::SimpleBuffer newbuf{};

    newbuf = std::move(orig);

    ASSERT_TRUE(newbuf.context.has_value());
    EXPECT_EQ(std::any_cast<std::string>(newbuf.context), "hello");
}

TEST(TestJsonBuffer, Clear)
{
    jsonwriter::SimpleBuffer out{};
    out.make_room(10);
    out.consume(std::copy_n(long_data.begin(), 10, out.working_end()));

    out.clear();

    EXPECT_EQ(out.size(), 0u);
    EXPECT_EQ(out.begin(), out.end());

    out.make_room(5);
    out.consume(std::copy_n(long_data.begin(), 5, out.working_end()));
    EXPECT_EQ((std::string_view{out.begin(), out.size()}), (std::string_view{long_data.data(), 5}));
}

//==========================================================================

TEST(TestJsonWriter, Char)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, char{'A'});
        EXPECT_EQ(to_str(out), "\"A\"");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, char{'"'});
        EXPECT_EQ(to_str(out), "\"\\\"\"");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, char{'\\'});
        EXPECT_EQ(to_str(out), "\"\\\\\"");
    }
}

TEST(TestJsonWriter, Integers)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 0);
        EXPECT_EQ(to_str(out), "0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -345);
        EXPECT_EQ(to_str(out), "-345");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, uint64_t{18446744073709551615ull});
        EXPECT_EQ(to_str(out), "18446744073709551615");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<int64_t>::max());
        EXPECT_EQ(to_str(out), "9223372036854775807");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<int64_t>::min());
        EXPECT_EQ(to_str(out), "-9223372036854775808");
    }

    const auto f = [](auto value) {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, value);
        EXPECT_EQ(to_str(out), "42");
    };

    f(int8_t{42});
    f(uint8_t{42});
    f(int16_t{42});
    f(uint16_t{42});
    f(int32_t{42});
    f(uint32_t{42});
    f(int64_t{42});
    f(uint64_t{42});

    f(static_cast<signed short>(42));
    f(static_cast<short>(42));
    f(static_cast<unsigned short>(42));

    f(static_cast<signed int>(42));
    f(static_cast<int>(42));
    f(static_cast<unsigned int>(42));

    f(static_cast<signed long>(42));
    f(static_cast<long>(42));
    f(static_cast<unsigned long>(42));

    f(static_cast<signed long int>(42));
    f(static_cast<long int>(42));
    f(static_cast<unsigned long int>(42));

    f(static_cast<signed long long>(42));
    f(static_cast<long long>(42));
    f(static_cast<unsigned long long>(42));

    f(static_cast<signed long int>(42));
    f(static_cast<long long int>(42));
    f(static_cast<unsigned long long int>(42));
}

TEST(TestJsonWriter, Floats)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 0.0);
        EXPECT_EQ(to_str(out), "0E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 3.5);
        EXPECT_EQ(to_str(out), "3.5E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 3.5f);
        EXPECT_EQ(to_str(out), "3.5E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 36.6472f);
        EXPECT_EQ(to_str(out), "3.66472E1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -123.456e-67);
        EXPECT_EQ(to_str(out), "-1.23456E-65");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 3.141592653589793);
        EXPECT_EQ(to_str(out), "3.141592653589793E0");
    }

    // negative zero keeps its sign
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -0.0);
        EXPECT_EQ(to_str(out), "-0E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -0.0f);
        EXPECT_EQ(to_str(out), "-0E0");
    }

    // negative values
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -1.0);
        EXPECT_EQ(to_str(out), "-1E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -1.0f);
        EXPECT_EQ(to_str(out), "-1E0");
    }

    // integral values are written in exponent notation, never with a trailing ".0"
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1.0);
        EXPECT_EQ(to_str(out), "1E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 100.0);
        EXPECT_EQ(to_str(out), "1E2");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1e15);
        EXPECT_EQ(to_str(out), "1E15");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1e16);
        EXPECT_EQ(to_str(out), "1E16");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1e21);
        EXPECT_EQ(to_str(out), "1E21");
    }

    // shortest representation that round-trips, so 0.3 becomes "3E-1"
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 0.1);
        EXPECT_EQ(to_str(out), "1E-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 0.3);
        EXPECT_EQ(to_str(out), "3E-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 0.3f);
        EXPECT_EQ(to_str(out), "3E-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 0.30000000000000004);
        EXPECT_EQ(to_str(out), "3.0000000000000004E-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1.0 / 3.0);
        EXPECT_EQ(to_str(out), "3.333333333333333E-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1.0f / 3.0f);
        EXPECT_EQ(to_str(out), "3.3333334E-1");
    }

    // smallest step above 1.0, the densest part of the exponent range
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1.0000000000000002); // nextafter(1.0, 2.0)
        EXPECT_EQ(to_str(out), "1.0000000000000002E0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 1.0000001f); // nextafter(1.0f, 2.0f)
        EXPECT_EQ(to_str(out), "1.0000001E0");
    }

    // integers beyond the mantissa width are rounded to the nearest representable value
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 9007199254740993.0); // 2^53 + 1, not representable
        EXPECT_EQ(to_str(out), "9.007199254740992E15");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 123456790.0f);
        EXPECT_EQ(to_str(out), "1.2345679E8");
    }

    // machine epsilon
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<double>::epsilon());
        EXPECT_EQ(to_str(out), "2.220446049250313E-16");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<float>::epsilon());
        EXPECT_EQ(to_str(out), "1.1920929E-7");
    }

    // largest finite values
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<double>::max());
        EXPECT_EQ(to_str(out), "1.7976931348623157E308");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<double>::lowest());
        EXPECT_EQ(to_str(out), "-1.7976931348623157E308");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<float>::max());
        EXPECT_EQ(to_str(out), "3.4028235E38");
    }

    // smallest normal values
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<double>::min());
        EXPECT_EQ(to_str(out), "2.2250738585072014E-308");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<float>::min());
        EXPECT_EQ(to_str(out), "1.1754944E-38");
    }

    // subnormals
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<double>::denorm_min());
        EXPECT_EQ(to_str(out), "5E-324");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::numeric_limits<float>::denorm_min());
        EXPECT_EQ(to_str(out), "1E-45");
    }

    // JSON cannot represent infinity or NaN, see JSONWRITER_NONFINITE
    const auto expect_nonfinite = [](const auto value) {
        jsonwriter::SimpleBuffer out{};
#if JSONWRITER_NONFINITE == JSONWRITER_NONFINITE_NULL
        jsonwriter::write(out, value);
        EXPECT_EQ(to_str(out), "null");
#elif JSONWRITER_NONFINITE == JSONWRITER_NONFINITE_ABORT
        EXPECT_DEATH(jsonwriter::write(out, value), "");
#else
        EXPECT_THROW(jsonwriter::write(out, value), std::domain_error);
#endif
    };
    expect_nonfinite(std::numeric_limits<double>::infinity());
    expect_nonfinite(-std::numeric_limits<double>::infinity());
    expect_nonfinite(std::numeric_limits<double>::quiet_NaN());
    expect_nonfinite(-std::numeric_limits<double>::quiet_NaN());
    expect_nonfinite(std::numeric_limits<double>::signaling_NaN());
    expect_nonfinite(std::numeric_limits<float>::infinity());
    expect_nonfinite(-std::numeric_limits<float>::infinity());
    expect_nonfinite(std::numeric_limits<float>::quiet_NaN());
    expect_nonfinite(-std::numeric_limits<float>::quiet_NaN());
    expect_nonfinite(std::numeric_limits<float>::signaling_NaN());
}

TEST(TestJsonWriter, Bool)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, true);
        EXPECT_EQ(to_str(out), "true");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, false);
        EXPECT_EQ(to_str(out), "false");
    }
}

/// Independent reference implementation of the string escaping rules: quote,
/// backslash and control characters are escaped, everything else is copied.
static std::string to_json_string(const std::string_view raw)
{
    std::string out{"\""};
    for (const char ch : raw) {
        const auto c = static_cast<unsigned char>(ch);
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20) {
                    constexpr char hex_digits[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex_digits[(c >> 4) & 0xf];
                    out += hex_digits[c & 0xf];
                } else {
                    out += ch;
                }
        }
    }
    out += '"';
    return out;
}

/// Buffer that allocates a sentinel filled guard band after its capacity, so a
/// write past the end of the reserved space is detectable without sanitizers.
class GuardedBuffer : public jsonwriter::Buffer
{
public:
    GuardedBuffer() { realloc(0, 8); }

    /// False if anything wrote past begin() + capacity(), now or before a
    /// reallocation.
    [[nodiscard]] bool ok() const noexcept { return !m_guard_violated && guard_intact(); }

protected:
    void realloc(const size_t data_size, const size_t new_capacity) override
    {
        // Catch an overrun of the storage that is about to be released.
        if (!guard_intact()) {
            m_guard_violated = true;
        }

        auto storage = std::make_unique<char[]>(new_capacity + GUARD_SIZE);
        ::memset(storage.get() + new_capacity, GUARD_BYTE, GUARD_SIZE);
        if (data_size > 0) {
            ::memcpy(storage.get(), begin(), data_size);
        }

        m_storage = std::move(storage);
        set_data(m_storage.get(), data_size, new_capacity);
    }

private:
    static constexpr size_t GUARD_SIZE{16};
    static constexpr unsigned char GUARD_BYTE{0xA5};

    [[nodiscard]] bool guard_intact() const noexcept
    {
        if (!m_storage) {
            return true;
        }
        for (size_t i{0}; i < GUARD_SIZE; ++i) {
            const auto byte = static_cast<unsigned char>(m_storage[capacity() + i]);
            if (byte != GUARD_BYTE) {
                return false;
            }
        }
        return true;
    }

    std::unique_ptr<char[]> m_storage;
    bool m_guard_violated{false};
};

TEST(TestJsonWriter, Strings)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, "");
        EXPECT_EQ(to_str(out), "\"\"");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, "ab\"\t\f\r\n\b\\de\x01\x1f ř\xff漢語zzz");
        EXPECT_EQ(to_str(out), "\"ab\\\"\\t\\f\\r\\n\\b\\\\de\\u0001\\u001f ř\xff漢語zzz\"");
    }
}

TEST(TestJsonWriter, StringEscapeEveryPosition)
{
    // One escape at every offset of a string spanning several bulk blocks, so
    // each scalar tail and SIMD block position is covered deterministically.
    for (const char escaped : {'"', '\\', '\x01'}) {
        for (size_t pos{0}; pos < 80; ++pos) {
            std::string raw(80, 'x');
            raw[pos] = escaped;

            jsonwriter::SimpleBuffer out{};
            jsonwriter::write(out, raw);
            EXPECT_EQ(to_str(out), to_json_string(raw)) << "pos=" << pos;
        }
    }
}

TEST(TestJsonWriter, StringsEscapingBoundaries)
{
    // Strings are written in fixed size bulk blocks (64 characters), so put a
    // two byte and a six byte escape around every block boundary and at the
    // very end of the input.
    constexpr size_t lengths[] = {1, 15, 16, 17, 63, 64, 65, 127, 128, 129};
    constexpr char escaped_chars[] = {'"', '\x01'};

    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::string_view{});
        EXPECT_EQ(to_str(out), "\"\"");
    }

    for (const size_t len : lengths) {
        for (const char escaped : escaped_chars) {
            for (const size_t escape_pos : {size_t{0}, len / 2, len - 1}) {
                std::string raw(len, 'x');
                raw[escape_pos] = escaped;

                jsonwriter::SimpleBuffer out{};
                jsonwriter::write(out, std::string_view{raw});
                EXPECT_EQ(to_str(out), to_json_string(raw))
                    << "len=" << len << " char=" << static_cast<int>(escaped)
                    << " escape_pos=" << escape_pos;
            }
        }
    }
}

TEST(TestJsonWriter, StringsEscapingAllBytes)
{
    // Includes the embedded NUL, the remaining control characters, ASCII and
    // bytes >= 0x80, which must be copied through unmodified.
    std::string raw;
    for (int i{0}; i < 256; ++i) {
        raw.push_back(static_cast<char>(i));
    }

    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, std::string_view{raw});
    EXPECT_EQ(to_str(out), to_json_string(raw));
}

TEST(TestJsonWriter, StringsEscapingWorstCaseBulk)
{
    // '\x01' always expands to the six byte `\u0001`, so a full bulk of 64 of
    // them is the widest write the formatter can make: the last escape starts
    // at 63 * 6 and the word sized copy adds EscapeMaps::MAX_LEN bytes.
    for (const size_t len : {size_t{64}, size_t{65}, size_t{128}, size_t{192}}) {
        const std::string raw(len, '\x01');

        GuardedBuffer out{};
        jsonwriter::write(out, std::string_view{raw});
        EXPECT_EQ(to_str(out), to_json_string(raw)) << "len=" << len;
        EXPECT_TRUE(out.ok()) << "len=" << len;
    }
}

TEST(TestJsonWriter, StringsEscapingRandom)
{
    // Differential test against the independent reference implementation, with
    // lengths that cross bulk boundaries and a deterministic random seed.
    std::mt19937 engine{12345};
    std::uniform_int_distribution<int> byte_dist{0, 255};
    std::uniform_int_distribution<size_t> len_dist{0, 300};

    for (int iteration{0}; iteration < 200; ++iteration) {
        std::string raw(len_dist(engine), '\0');
        for (auto& ch : raw) {
            ch = static_cast<char>(byte_dist(engine));
        }

        jsonwriter::SimpleBuffer<8> out{};
        jsonwriter::write(out, std::string_view{raw});
        ASSERT_EQ(to_str(out), to_json_string(raw)) << "iteration=" << iteration;
    }
}

TEST(TestJsonWriter, StringsEscapingSmallBuffer)
{
    // Force several reallocations while writing escaped data, then keep using
    // the same buffer.
    std::string raw;
    for (int i{0}; i < 300; ++i) {
        raw.push_back(static_cast<char>(i % 0x20));
    }

    jsonwriter::SimpleBuffer<8> out{};
    jsonwriter::write(out, std::string_view{raw});
    EXPECT_EQ(to_str(out), to_json_string(raw));

    jsonwriter::write(out, "a\tb");
    EXPECT_EQ(to_str(out), to_json_string(raw) + to_json_string("a\tb"));
}

TEST(TestJsonWriter, Optional)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::nullopt);
        EXPECT_EQ(to_str(out), "null");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<int>{});
        EXPECT_EQ(to_str(out), "null");
    }

    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<std::string>{});
        EXPECT_EQ(to_str(out), "null");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<int>{2349});
        EXPECT_EQ(to_str(out), "2349");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<std::string>{"abc"});
        EXPECT_EQ(to_str(out), "\"abc\"");
    }

    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<bool>{});
        EXPECT_EQ(to_str(out), "null");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<bool>{false});
        EXPECT_EQ(to_str(out), "false");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::optional<bool>{true});
        EXPECT_EQ(to_str(out), "true");
    }
}

TEST(TestJsonWriter, ListLikeContainers)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::empty_list);
        EXPECT_EQ(to_str(out), "[]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        // initializer list
        jsonwriter::write(out, {33, 44});
        EXPECT_EQ(to_str(out), "[33,44]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::initializer_list<int>{33, 44});
        EXPECT_EQ(to_str(out), "[33,44]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, {"ab", "c", "de\rf"});
        EXPECT_EQ(to_str(out), "[\"ab\",\"c\",\"de\\rf\"]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::vector<std::string>{"ab", "c", "de\rf"});
        EXPECT_EQ(to_str(out), "[\"ab\",\"c\",\"de\\rf\"]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::array<int, 3>{{33, 44, 999}});
        EXPECT_EQ(to_str(out), "[33,44,999]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::deque<int>{33, 44, 999});
        EXPECT_EQ(to_str(out), "[33,44,999]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::forward_list<int>{33, 44, 999});
        EXPECT_EQ(to_str(out), "[33,44,999]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::list<int>{33, 44, 999});
        EXPECT_EQ(to_str(out), "[33,44,999]");
    }
}

TEST(TestJsonWriter, ScalarListChunkBoundaries)
{
    const auto expected = [](const size_t count) {
        std::string s{"["};
        for (size_t i{0}; i < count; ++i) {
            if (i != 0) {
                s += ',';
            }
            s += std::to_string(i);
        }
        s += ']';
        return s;
    };

    for (const size_t count : {0u, 1u, 255u, 256u, 257u, 1000u}) {
        std::vector<int> values(count);
        for (size_t i{0}; i < count; ++i) {
            values[i] = static_cast<int>(i);
        }

        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, values);
        EXPECT_EQ(to_str(out), expected(count)) << "count=" << count;
    }
}

TEST(TestJsonWriter, ScalarListValues)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::vector<bool>{true, false, true});
        EXPECT_EQ(to_str(out), "[true,false,true]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::vector<double>{1.5, -2.25});
        EXPECT_EQ(to_str(out), "[1.5E0,-2.25E0]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::array<float, 2>{{0.5f, 3.5f}});
        EXPECT_EQ(to_str(out), "[5E-1,3.5E0]");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::list<unsigned>{1, 2});
        EXPECT_EQ(to_str(out), "[1,2]");
    }
    {
        // char keeps its string semantics and must not take the numeric path
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, std::vector<char>{'a', 'b'});
        EXPECT_EQ(to_str(out), "[\"a\",\"b\"]");
    }
}

TEST(TestJsonWriter, ScalarListNonFinite)
{
    const std::vector<double> values{1.0, std::numeric_limits<double>::infinity(), 2.0};
    jsonwriter::SimpleBuffer out{};
#if JSONWRITER_NONFINITE == JSONWRITER_NONFINITE_NULL
    jsonwriter::write(out, values);
    EXPECT_EQ(to_str(out), "[1E0,null,2E0]");
#elif JSONWRITER_NONFINITE == JSONWRITER_NONFINITE_ABORT
    EXPECT_DEATH(jsonwriter::write(out, values), "");
#else
    EXPECT_THROW(jsonwriter::write(out, values), std::domain_error);
#endif
}

TEST(TestJsonWriter, List)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::List([](auto& list) {
                              list.push_back(33);
                              list.push_back("abc");
                              list.push_back(true);
                              list.push_back({1, 2, 3});
                          }));
        EXPECT_EQ(to_str(out), "[33,\"abc\",true,[1,2,3]]");
    }
}

static void add_keys(jsonwriter::ObjectProxy& object) { object["aaa"] = "bbb"; }

static auto get_object()
{
    jsonwriter::Object obj{[](auto& object) { object["x"] = 5; }};
    return obj;
}

TEST(TestJsonWriter, Objects)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::empty_object);
        EXPECT_EQ(to_str(out), "{}");
    }
    {
        int some_value{42};
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::Object{[some_value](auto& object) {
                              object["k1"] = "c\tdžř漢語";
                              object["k\n2"] = {3, 5, 6};
                              object["k3"] = 87;
                              object["k4"] = {"\\"};
                              object["k5"] = some_value;
                              object["k6"] = 3.5;
                          }});
        EXPECT_EQ(to_str(out), "{\"k1\":\"c\\tdžř漢語\",\"k\\n2\":[3,5,6],\"k3\":87,\"k4\":["
                               "\"\\\\\"],\"k5\":42,\"k6\":3.5E0}");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::Object{[](auto& object) {
                              object["k1"] = "cd";
                              object["k2"] = jsonwriter::Object{[](auto& nested_object) {
                                  nested_object["o1"] = {1, 2};
                                  nested_object["o2"] = false;
                                  nested_object["o\r3"] = "i\no";
                              }};
                              object["k3"] = false;
                          }});
        EXPECT_EQ(to_str(out), "{\"k1\":\"cd\",\"k2\":{\"o1\":[1,2],\"o2\":false,\"o\\r3\":"
                               "\"i\\no\"},\"k3\":false}");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::Object{[](auto& object) { add_keys(object); }});
        EXPECT_EQ(to_str(out), "{\"aaa\":\"bbb\"}");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, get_object());
        EXPECT_EQ(to_str(out), "{\"x\":5}");
    }
}

TEST(TestJsonWriter, ListOutlivesCallbackTemporary)
{
    std::weak_ptr<int> lifetime;
    const auto make_list = [&lifetime] {
        const auto token = std::make_shared<int>(42);
        lifetime = token;
        return jsonwriter::List{[token](auto& list) { list.push_back(*token); }};
    };

    const auto list = make_list();
    EXPECT_FALSE(lifetime.expired());

    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, list);
    EXPECT_EQ(to_str(out), "[42]");
}

TEST(TestJsonWriter, ObjectOutlivesCallbackTemporary)
{
    std::weak_ptr<int> lifetime;
    const auto make_object = [&lifetime] {
        const auto token = std::make_shared<int>(42);
        lifetime = token;
        return jsonwriter::Object{[token](auto& object) { object["k"] = *token; }};
    };

    const auto object = make_object();
    EXPECT_FALSE(lifetime.expired());

    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, object);
    EXPECT_EQ(to_str(out), "{\"k\":42}");
}

enum class SomeEnum { RED, GREEN, BLUE };

template<>
struct jsonwriter::Formatter<SomeEnum>
{
    static void write(Buffer& buffer, const SomeEnum value)
    {
        switch (value) {
            case SomeEnum::RED:
                buffer.append("red");
                return;
            case SomeEnum::GREEN:
                buffer.append("green");
                return;
            case SomeEnum::BLUE:
                buffer.append("blue");
                return;
            default:
                buffer.append("n/a");
                return;
        }
    }
};

TEST(TestJsonWriter, CustomEnumLabel)
{
    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, SomeEnum::GREEN);
    EXPECT_EQ(to_str(out), "green");
}

struct SomeStruct
{
    int a{42};
};

template<>
struct jsonwriter::Formatter<SomeStruct>
{
    static void write(Buffer& buffer, const SomeStruct& value)
    {
        jsonwriter::write(buffer, jsonwriter::Object{[&value, &buffer](auto& object) {
                              object["a"] = value.a;
                              object["ctx"] = std::any_cast<std::string>(buffer.context);
                          }});
    }
};

TEST(TestJsonWriter, CustomStructWithContext)
{
    jsonwriter::SimpleBuffer out{};
    out.context = std::string{"hello"};
    jsonwriter::write(out, SomeStruct{});
    EXPECT_EQ(to_str(out), "{\"a\":42,\"ctx\":\"hello\"}");
}

TEST(TestJsonWriter, CustomStructWithBuiltInFormatter)
{
    struct OtherStruct
    {
        int b{666};

        void write(jsonwriter::Buffer& buffer)
        {
            jsonwriter::write(buffer,
                              jsonwriter::Object{[this](auto& object) { object["b"] = b; }});
        }
    };

    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, OtherStruct{});
    EXPECT_EQ(to_str(out), "{\"b\":666}");
}

TEST(TestJsonWriter, CustomStructWithBuiltInFormatter_Const)
{
    struct OtherStruct
    {
        int b{666};

        void write(jsonwriter::Buffer& buffer) const
        {
            jsonwriter::write(buffer,
                              jsonwriter::Object{[this](auto& object) { object["b"] = b; }});
        }
    };

    jsonwriter::SimpleBuffer out{};
    const OtherStruct s{};
    jsonwriter::write(out, s);
    EXPECT_EQ(to_str(out), "{\"b\":666}");
}
