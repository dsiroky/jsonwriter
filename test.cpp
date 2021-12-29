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

namespace {

TEST(TestJsonWriter, Integers)
{
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), 0);
        EXPECT_EQ(out, "0");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), -345);
        EXPECT_EQ(out, "-345");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), uint64_t{18446744073709551615ull});
        EXPECT_EQ(out, "18446744073709551615");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), std::numeric_limits<int64_t>::max());
        EXPECT_EQ(out, "9223372036854775807");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), std::numeric_limits<int64_t>::min());
        EXPECT_EQ(out, "-9223372036854775808");
    }
}

TEST(TestJsonWriter, Floats)
{
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), 0.0);
        EXPECT_EQ(out, "0");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), 3.5);
        EXPECT_EQ(out, "35e-1");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), 3.5f);
        EXPECT_EQ(out, "35e-1");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), -123.456e-67);
        EXPECT_EQ(out, "-123456e-70");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), M_PI);
        EXPECT_EQ(out, "3141592653589793e-15");
    }
}

TEST(TestJsonWriter, Bool)
{
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), true);
        EXPECT_EQ(out, "true");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), false);
        EXPECT_EQ(out, "false");
    }
}

TEST(TestJsonWriter, Strings)
{
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), "");
        EXPECT_EQ(out, "\"\"");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), "ab\"\t\f\r\n\b\\de\x01\x1f ř\xff漢語zzz");
        EXPECT_EQ(out, "\"ab\\\"\\t\\f\\r\\n\\b\\\\de\\u0001\\u001f ř\xff漢語zzz\"");
    }
}

TEST(TestJsonWriter, ListLikeContainers)
{
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), {33, 44});
        EXPECT_EQ(out, "[33,44]");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), {"ab", "c", "de\rf"});
        EXPECT_EQ(out, "[\"ab\",\"c\",\"de\\rf\"]");
    }
    {
        std::string out{};
        jsonwriter::write(std::back_inserter(out), std::vector<std::string>{"ab", "c", "de\rf"});
        EXPECT_EQ(out, "[\"ab\",\"c\",\"de\\rf\"]");
    }
}

TEST(TestJsonWriter, Objects)
{
    {
        int some_value{42};
        std::string out{};
        jsonwriter::write(
            std::back_inserter(out),
            [some_value](jsonwriter::Object<std::back_insert_iterator<std::string>>& object) {
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
        jsonwriter::write(std::back_inserter(out), [](auto& object) {
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

TEST(TestJsonWriter, CheckWorkWithGenericIterators)
{
    std::array<char, 100> out{};
    // no back inserter
    auto end = jsonwriter::write(out.begin(),
        [](jsonwriter::Object<decltype(out)::iterator>& object) {
              object["k1"] = "c\td";
              object["k\n2"] = {3, 5, 6};
              object["k3"] = 87;
              object["k4"] = {"\\"};
              object["k5"] = 3.5;
              object["k6"] = true;
          });
    EXPECT_EQ((std::string{out.begin(), static_cast<size_t>(std::distance(out.begin(), end))}),
      "{\"k1\":\"c\\td\",\"k\\n2\":[3,5,6],\"k3\":87,\"k4\":[\"\\\\\"],\"k5\":35e-1,\"k6\":true}");
}

} //
