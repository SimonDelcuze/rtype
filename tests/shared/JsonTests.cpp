#include "json/Json.hpp"
#include <gtest/gtest.h>

using namespace rtype;

TEST(JsonTest, ParseAndDump)
{
    std::string jsonStr = R"({"key": "value", "number": 42})";
    Json json           = Json::parse(jsonStr);

    EXPECT_TRUE(json.contains("key"));
    EXPECT_TRUE(json.contains("number"));
    EXPECT_EQ(json.getValue<std::string>("key"), "value");
    EXPECT_EQ(json.getValue<int>("number"), 42);

    std::string dumped = json.dump();
    EXPECT_NE(dumped.find("value"), std::string::npos);
}

TEST(JsonTest, TypeMismatch)
{
    std::string jsonStr = R"({"key": "value"})";
    Json json           = Json::parse(jsonStr);

    EXPECT_THROW(json.getValue<int>("key"), JsonTypeError);
    EXPECT_THROW(json.getValue<std::string>("nonexistent"), JsonKeyError);
}

TEST(JsonTest, DefaultValue)
{
    std::string jsonStr = R"({"key": "value"})";
    Json json           = Json::parse(jsonStr);

    EXPECT_EQ(json.getValue<int>("nonexistent", 10), 10);
    EXPECT_EQ(json.getValue<std::string>("key", "default"), "value");
}

TEST(JsonTest, ArrayOperations)
{
    Json arr = Json::array();

    Json obj1 = Json::object();
    obj1.setValue("id", 1);

    Json obj2 = Json::object();
    obj2.setValue("id", 2);

    arr.pushBack(obj1);
    arr.pushBack(obj2);

    EXPECT_EQ(arr.size(), 2);
    EXPECT_TRUE(arr.isArray());
    EXPECT_FALSE(arr.isObject());

    Json item0 = arr[0];
    EXPECT_EQ(item0.getValue<int>("id"), 1);

    Json item1 = arr[1];
    EXPECT_EQ(item1.getValue<int>("id"), 2);
}

TEST(JsonTest, InvalidParse)
{
    std::string invalidJson = "{ invalid }";
    EXPECT_THROW(Json::parse(invalidJson), JsonParseError);
}

TEST(JsonTest, NestedAccess)
{
    std::string jsonStr = R"({"parent": {"child": "hello"}})";
    Json json           = Json::parse(jsonStr);

    EXPECT_TRUE(json.contains("parent"));
    Json parent = json["parent"];
    EXPECT_TRUE(parent.contains("child"));
    EXPECT_EQ(parent.getValue<std::string>("child"), "hello");
}
