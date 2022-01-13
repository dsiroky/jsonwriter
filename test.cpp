#include <iterator>
#include <vector>

#include <gtest/gtest.h>

#include "jsonwriter/writer.hpp"

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

//==========================================================================

TEST(TestJsonWriter, Char)
{
    {
        std::string out{};
        jsonwriter::write(out, char{'A'});
        EXPECT_EQ(out, "\"A\"");
    }
}

TEST(TestJsonWriter, Integers)
{
    {
        std::string out{};
        jsonwriter::write(out, 0);
        EXPECT_EQ(out, "0");
    }
    {
        std::string out{};
        jsonwriter::write(out, -345);
        EXPECT_EQ(out, "-345");
    }
    {
        std::string out{};
        jsonwriter::write(out, uint64_t{18446744073709551615ull});
        EXPECT_EQ(out, "18446744073709551615");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::numeric_limits<int64_t>::max());
        EXPECT_EQ(out, "9223372036854775807");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::numeric_limits<int64_t>::min());
        EXPECT_EQ(out, "-9223372036854775808");
    }

    const auto f = [](auto value) {
        std::string out{};
        jsonwriter::write(out, value);
        EXPECT_EQ(out, "42");
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
        std::string out{};
        jsonwriter::write(out, 0.0);
        EXPECT_EQ(out, "0");
    }
    {
        std::string out{};
        jsonwriter::write(out, 3.5);
        EXPECT_EQ(out, "35e-1");
    }
    {
        std::string out{};
        jsonwriter::write(out, 3.5f);
        EXPECT_EQ(out, "35e-1");
    }
    {
        std::string out{};
        jsonwriter::write(out, -123.456e-67);
        EXPECT_EQ(out, "-123456e-70");
    }
    {
        std::string out{};
        jsonwriter::write(out, M_PI);
        EXPECT_EQ(out, "3141592653589793e-15");
    }
}

TEST(TestJsonWriter, Bool)
{
    {
        std::string out{};
        jsonwriter::write(out, true);
        EXPECT_EQ(out, "true");
    }
    {
        std::string out{};
        jsonwriter::write(out, false);
        EXPECT_EQ(out, "false");
    }
}

TEST(TestJsonWriter, Strings)
{
    {
        std::string out{};
        jsonwriter::write(out, "");
        EXPECT_EQ(out, "\"\"");
    }
    {
        std::string out{};
        jsonwriter::write(out, "ab\"\t\f\r\n\b\\de\x01\x1f ř\xff漢語zzz");
        EXPECT_EQ(out, "\"ab\\\"\\t\\f\\r\\n\\b\\\\de\\u0001\\u001f ř\xff漢語zzz\"");
    }
}

TEST(TestJsonWriter, Optional)
{
    {
        std::string out{};
        jsonwriter::write(out, std::nullopt);
        EXPECT_EQ(out, "null");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::optional<int>{});
        EXPECT_EQ(out, "null");
    }

    {
        std::string out{};
        jsonwriter::write(out, std::optional<std::string>{});
        EXPECT_EQ(out, "null");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::optional<int>{2349});
        EXPECT_EQ(out, "2349");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::optional<std::string>{"abc"});
        EXPECT_EQ(out, "\"abc\"");
    }

    {
        std::string out{};
        jsonwriter::write(out, std::optional<bool>{});
        EXPECT_EQ(out, "null");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::optional<bool>{false});
        EXPECT_EQ(out, "false");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::optional<bool>{true});
        EXPECT_EQ(out, "true");
    }
}

TEST(TestJsonWriter, ListLikeContainers)
{
    {
        std::string out{};
        // initializer list
        jsonwriter::write(out, {33, 44});
        EXPECT_EQ(out, "[33,44]");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::initializer_list<int>{33, 44});
        EXPECT_EQ(out, "[33,44]");
    }
    {
        std::string out{};
        jsonwriter::write(out, {"ab", "c", "de\rf"});
        EXPECT_EQ(out, "[\"ab\",\"c\",\"de\\rf\"]");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::vector<std::string>{"ab", "c", "de\rf"});
        EXPECT_EQ(out, "[\"ab\",\"c\",\"de\\rf\"]");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::array<int, 3>{{33, 44, 999}});
        EXPECT_EQ(out, "[33,44,999]");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::deque<int>{33, 44, 999});
        EXPECT_EQ(out, "[33,44,999]");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::forward_list<int>{33, 44, 999});
        EXPECT_EQ(out, "[33,44,999]");
    }
    {
        std::string out{};
        jsonwriter::write(out, std::list<int>{33, 44, 999});
        EXPECT_EQ(out, "[33,44,999]");
    }
}

TEST(TestJsonWriter, Objects)
{
    {
        int some_value{42};
        std::string out{};
        jsonwriter::write(
            out,
            [some_value](jsonwriter::Object<std::string>& object) {
                object["k1"] = "c\td";
                object["k\n2"] = {3, 5, 6};
                object["k3"] = 87;
                object["k4"] = {"\\"};
                object["k5"] = some_value;
            });
        EXPECT_EQ(out,
                  "{\"k1\":\"c\\td\",\"k\\n2\":[3,5,6],\"k3\":87,\"k4\":[\"\\\\\"],\"k5\":42}");
    }
    {
        std::string out{};
        jsonwriter::write(out, [](auto& object) {
            object["k1"] = "cd";
            object["k2"] = [](auto& nested_object) {
                nested_object["o1"] = {1, 2};
                nested_object["o2"] = false;
                nested_object["o\r3"] = "i\no";
            };
            object["k3"] = false;
        });
        EXPECT_EQ(out, "{\"k1\":\"cd\",\"k2\":{\"o1\":[1,2],\"o2\":false,\"o\\r3\":"
                       "\"i\\no\"},\"k3\":false}");
    }
}

enum class SomeEnum { RED, GREEN, BLUE };

template<>
struct jsonwriter::Formatter<SomeEnum>
{
    template<typename Output>
    static void write(Output& output, const SomeEnum value)
    {
        TailBuffer tail{output};
        switch (value) {
            case SomeEnum::RED:
                tail.append("red");
                return;
            case SomeEnum::GREEN:
                tail.append("green");
                return;
            case SomeEnum::BLUE:
                tail.append("blue");
                return;
            default:
                tail.append("n/a");
                return;
        }
    }
};

TEST(TestJsonWriter, CustomEnumLabel)
{
    std::string out{};
    jsonwriter::write(out, SomeEnum::GREEN);
    EXPECT_EQ(out, "green");
}

struct SomeStruct
{
    int a{42};
};

template<>
struct jsonwriter::Formatter<SomeStruct>
{
    template<typename Output>
    static void write(Output& output, const SomeStruct& value)
    {
        jsonwriter::write(output, [&value](auto& object) { object["a"] = value.a; });
    }
};

TEST(TestJsonWriter, CustomStruct)
{
    std::string out{};
    jsonwriter::write(out, SomeStruct{});
    EXPECT_EQ(out, "{\"a\":42}");
}
