#include "catch2/catch.hpp"
#include "configKeyValuePairs.h"

TEST_CASE("Test configKeyValuePairs function", "[configKeyValuePairs]") {
  using namespace cce::tf;
  SECTION("empty config") {
    REQUIRE(configKeyValuePairs("").empty());
  }
  SECTION("single item no value"){
    auto v = configKeyValuePairs("foo");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "foo");
    REQUIRE( v.begin()->second.empty());
  }
  SECTION("single file name") {
    auto v = configKeyValuePairs("foo.root");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "fileName");
    REQUIRE( v.begin()->second == "foo.root");
  }
  SECTION("single file name with quotes") {
    auto v = configKeyValuePairs("\"foo.root\"");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "fileName");
    REQUIRE( v.begin()->second == "foo.root");
  }
  SECTION("single item with value") {
    auto v = configKeyValuePairs("foo=3");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "foo");
    REQUIRE( v.begin()->second == "3");
  }
  SECTION("single item with quoted value") {
    auto v = configKeyValuePairs("foo=\"3\"");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "foo");
    REQUIRE( v.begin()->second == "3");
  }
  SECTION("single item with quoted value containing :") {
    auto v = configKeyValuePairs("foo=\"http://foo.com\"");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "foo");
    REQUIRE( v.begin()->second == "http://foo.com");
  }
  SECTION("single item with empty value") {
    auto v = configKeyValuePairs("foo=");
    REQUIRE(v.size() == 1);
    REQUIRE( v.begin()->first == "foo");
    REQUIRE( v.begin()->second.empty());
  }
  SECTION("two items with no values") {
    auto v = configKeyValuePairs("foo:bar");
    REQUIRE(v.size() == 2);    
    auto it = v.begin();
    REQUIRE( it->first == "bar");
    REQUIRE( it->second.empty());
    ++it;
    REQUIRE( it->first == "foo");
    REQUIRE( it->second.empty());
  }
  SECTION("two items with values") {
    auto v = configKeyValuePairs("foo=3:bar=cat");
    REQUIRE(v.size() == 2);    
    auto it = v.begin();
    REQUIRE( it->first == "bar");
    REQUIRE( it->second == "cat");
    ++it;
    REQUIRE( it->first == "foo");
    REQUIRE( it->second == "3");
  }
  SECTION("two items, first with values, second without") {
    auto v = configKeyValuePairs("foo=3:bar");
    REQUIRE(v.size() == 2);    
    auto it = v.begin();
    REQUIRE( it->first == "bar");
    REQUIRE( it->second.empty());
    ++it;
    REQUIRE( it->first == "foo");
    REQUIRE( it->second == "3");
  }
  SECTION("two items, first without value, second with") {
    auto v = configKeyValuePairs("foo:bar=cat");
    REQUIRE(v.size() == 2);    
    auto it = v.begin();
    REQUIRE( it->first == "bar");
    REQUIRE( it->second == "cat");
    ++it;
    REQUIRE( it->first == "foo");
    REQUIRE( it->second.empty());
  }

}
