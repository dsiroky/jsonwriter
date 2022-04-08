#include <iterator>
#include <vector>

#include <gtest/gtest.h>

#include "jsonwriter/writer.hpp"

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

static std::string to_str(const jsonwriter::Buffer& b) { return std::string{b.begin(), b.end()}; }

static std::vector<char> long_data{std::invoke([](){
            std::vector<char> v(10000);
            for (size_t i{0}; i < v.size(); ++i) {
                v[i] = static_cast<char>(i);
            }
            return v;
        })};

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
            EXPECT_EQ((std::string_view{newbuf.data(), 10}), (std::string_view{long_data.data(), 10}));

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
            EXPECT_EQ((std::string_view{newbuf.data(), 10}), (std::string_view{long_data.data(), 10}));

            // this should be undefined, nullptr test is a white box test
            EXPECT_TRUE(orig.begin() == nullptr);
        }
    }
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
        EXPECT_EQ(to_str(out), "0");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 3.5);
        EXPECT_EQ(to_str(out), "35e-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 3.5f);
        EXPECT_EQ(to_str(out), "35e-1");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, -123.456e-67);
        EXPECT_EQ(to_str(out), "-123456e-70");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, 3.141592653589793);
        EXPECT_EQ(to_str(out), "3141592653589793e-15");
    }
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

TEST(TestJsonWriter, List)
{
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, jsonwriter::List([](auto& list){
            list.push_back(33);
            list.push_back("abc");
            list.push_back(true);
            list.push_back({1, 2, 3});
        }));
        EXPECT_EQ(to_str(out), "[33,\"abc\",true,[1,2,3]]");
    }
}

static void add_keys(jsonwriter::ObjectProxy& object)
{
    object["aaa"] = "bbb";
}

static auto get_object()
{
    jsonwriter::Object obj{[](auto& object) { object["x"] = 5; }};
    return obj;
}

TEST(TestJsonWriter, Objects)
{
    {
        int some_value{42};
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(
            out,
            jsonwriter::Object{[some_value](auto& object) {
                object["k1"] = "c\tdžř漢語";
                object["k\n2"] = {3, 5, 6};
                object["k3"] = 87;
                object["k4"] = {"\\"};
                object["k5"] = some_value;
                object["k6"] = 3.5;
            }});
        EXPECT_EQ(to_str(out), "{\"k1\":\"c\\tdžř漢語\",\"k\\n2\":[3,5,6],\"k3\":87,\"k4\":["
                               "\"\\\\\"],\"k5\":42,\"k6\":35e-1}");
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
        jsonwriter::write(out, jsonwriter::Object{[](auto& object) {
            add_keys(object);
        }});
        EXPECT_EQ(to_str(out), "{\"aaa\":\"bbb\"}");
    }
    {
        jsonwriter::SimpleBuffer out{};
        jsonwriter::write(out, get_object());
        EXPECT_EQ(to_str(out), "{\"x\":5}");
    }
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
        jsonwriter::write(buffer,
                          jsonwriter::Object{[&value](auto& object) { object["a"] = value.a; }});
    }
};

TEST(TestJsonWriter, CustomStruct)
{
    jsonwriter::SimpleBuffer out{};
    jsonwriter::write(out, SomeStruct{});
    EXPECT_EQ(to_str(out), "{\"a\":42}");
}
