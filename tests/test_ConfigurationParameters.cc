#include "catch2/catch.hpp"
#include "ConfigurationParameters.h"

TEST_CASE("Test ConfigurationParameters class", "[ConfigurationParameters]") {
  using namespace cce::tf;

  SECTION("empty configuration") {
    ConfigurationParameters::KeyValueMap map;
    ConfigurationParameters params(map);
    SECTION("unusedKeys") {
      REQUIRE(params.unusedKeys().empty());
    }
    SECTION("get with out default") {
      REQUIRE(not params.get<int>("foo"));
      REQUIRE(not params.get<std::string>("foo"));
      REQUIRE(not params.get<unsigned int>("foo"));
      REQUIRE(not params.get<bool>("foo"));
    }
    SECTION("defaults for missing parameters") {
      REQUIRE(params.get<int>("foo", 1) == 1);
      REQUIRE(params.get<unsigned int>("foo", 10) == 10);
      REQUIRE(params.get<std::string>("foo","bar") == "bar");
      REQUIRE(params.get<bool>("foo",true));
      REQUIRE(not params.get<bool>("foo", false));
    }
  }


  SECTION("string param") {
    ConfigurationParameters::KeyValueMap map = {{"foo", "bar"}};
    ConfigurationParameters params(map);
    REQUIRE(params.unusedKeys().size() == 1);
    REQUIRE(params.get<std::string>("foo") == "bar");
    REQUIRE(params.get<std::string>("foo", "blah") == "bar");
    REQUIRE(params.unusedKeys().empty());
  }

  SECTION("int param") {
    ConfigurationParameters::KeyValueMap map = {{"foo", "-3"}};
    ConfigurationParameters params(map);
    REQUIRE(params.unusedKeys().size() == 1);
    REQUIRE(params.get<int>("foo") == -3);
    REQUIRE(params.get<int>("foo", 1) == -3);
    REQUIRE(params.unusedKeys().empty());
  }

  SECTION("unsigned int param") {
    ConfigurationParameters::KeyValueMap map = {{"foo", "5"}};
    ConfigurationParameters params(map);
    REQUIRE(params.unusedKeys().size() == 1);
    REQUIRE(params.get<unsigned int>("foo") == 5);
    REQUIRE(params.get<unsigned int>("foo", 1) == 5);
    REQUIRE(params.unusedKeys().empty());
  }

  SECTION("bool param") {
    SECTION("true") {
      ConfigurationParameters::KeyValueMap map = {{"a", "T"}, {"b", "t"}, {"c", "Y"}, {"d", "y"}, {"e",""}, 
                                                  {"f", "TRUE"}, {"g", "True"}, {"h", "true"},
                                                  {"i", "YES"}, {"j", "Yes"}, {"k", "yes"}};
      ConfigurationParameters params(map);
      REQUIRE(*params.get<bool>("a"));
      REQUIRE(*params.get<bool>("b"));
      REQUIRE(*params.get<bool>("c"));
      REQUIRE(*params.get<bool>("d"));
      REQUIRE(*params.get<bool>("e"));
      REQUIRE(*params.get<bool>("f"));
      REQUIRE(*params.get<bool>("g"));
      REQUIRE(*params.get<bool>("h"));
      REQUIRE(*params.get<bool>("i"));
      REQUIRE(*params.get<bool>("j"));
      REQUIRE(*params.get<bool>("k"));
    }
    SECTION("false") {
      ConfigurationParameters::KeyValueMap map = {{"a", "F"}, {"b", "f"}, {"c", "N"}, {"d", "n"}, 
                                                  {"f", "FALSE"}, {"g", "False"}, {"h", "false"},
                                                  {"i", "NO"}, {"j", "No"}, {"k", "no"}};
      ConfigurationParameters params(map);
      REQUIRE(not *params.get<bool>("a"));
      REQUIRE(not *params.get<bool>("b"));
      REQUIRE(not *params.get<bool>("c"));
      REQUIRE(not *params.get<bool>("d"));
      REQUIRE(not *params.get<bool>("f"));
      REQUIRE(not *params.get<bool>("g"));
      REQUIRE(not *params.get<bool>("h"));
      REQUIRE(not *params.get<bool>("i"));
      REQUIRE(not *params.get<bool>("j"));
      REQUIRE(not *params.get<bool>("k"));
    }
  }

}
