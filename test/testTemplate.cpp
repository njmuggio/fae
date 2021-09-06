/**
 * @file testTemplate.cpp
 * @brief Tests for fae::Template
 * @author Nick Muggio
 */

#include <deque>
#include <map>
#include <string>
#include <variant>

#include "catch2/catch.hpp"

#include "fae.hpp"

TEST_CASE("default constructor produces empty output", "[template]")
{
  fae::Template tmpl;

  std::map<std::string, std::variant<int>> input;

  SECTION("empty input map")
  {
    REQUIRE(tmpl.render(input) == "");
  }

  SECTION("populated input map")
  {
    input["soup"] = 123;

    REQUIRE(tmpl.render(input) == "");
  }
}

TEST_CASE("static templates", "[template]")
{
  fae::Template tmpl("Expressionless :|");

  std::map<std::string, std::variant<int>> input;

  REQUIRE(tmpl.render(input) == "Expressionless :|");
}

TEST_CASE("basic variable substitution", "[template]")
{
  fae::Template tmpl("someVal: $(someVal)");

  std::map<std::string, std::variant<int, bool, std::string>> input;

  SECTION("int substitution")
  {
    input["someVal"] = 123;

    REQUIRE(tmpl.render(input) == "someVal: 123");
  }

  SECTION("bool substitution")
  {
    input["someVal"] = true;

    REQUIRE(tmpl.render(input) == "someVal: true");
  }

  SECTION("string substitution")
  {
    input["someVal"] = "indeed";

    REQUIRE(tmpl.render(input) == "someVal: indeed");
  }

  SECTION("missing substitution")
  {
    REQUIRE(tmpl.render(input) == "someVal: ");
  }
}

TEST_CASE("expression and escape escaping", "[template]")
{
  std::map<std::string, std::variant<int>> input{{"val", 5}};

  SECTION("escaped expression")
  {
    fae::Template tmpl(R"(\$(val))");

    REQUIRE(tmpl.render(input) == "$(val)");
  }

  SECTION("escaped escape before expression")
  {
    fae::Template tmpl(R"(\\$(val))");

    REQUIRE(tmpl.render(input) == R"(\5)");
  }

  SECTION("backslash before escaped expression")
  {
    fae::Template tmpl(R"(\\\$(val))");

    REQUIRE(tmpl.render(input) == R"(\\5)");
  }

  SECTION("escaped expression after regular text")
  {
    fae::Template tmpl(R"(2+3=\$(val))");

    REQUIRE(tmpl.render(input) == R"(2+3=$(val))");
  }

  SECTION("escaped escape after regular text")
  {
    fae::Template tmpl(R"(2+3=\\$(val))");

    REQUIRE(tmpl.render(input) == R"(2+3=\5)");
  }

  SECTION("backslash before escaped expression after regular text")
  {
    fae::Template tmpl(R"(2+3=\\\$(val))");

    REQUIRE(tmpl.render(input) == R"(2+3=\\5)");
  }
}

TEST_CASE("if expression", "[template]")
{
  std::map<std::string, std::variant<bool, int, std::string>> input;

  input["bTrue"] = true;
  input["bFalse"] = false;
  input["i0"] = 0;
  input["i5"] = 5;
  input["sEmpty"] = "";
  input["sFull"] = "full";

  SECTION("missing field")
  {
    fae::Template tmpl("$(if iDontExist)found$(end)");

    REQUIRE(tmpl.render(input) == "");
  }

  SECTION("true bool")
  {
    fae::Template tmpl("$(if bTrue)found$(end)");

    REQUIRE(tmpl.render(input) == "found");
  }

  SECTION("false bool")
  {
    fae::Template tmpl("$(if bFalse)found$(end)");

    REQUIRE(tmpl.render(input) == "found");
  }

  SECTION("zero int")
  {
    fae::Template tmpl("$(if i0)found$(end)");

    REQUIRE(tmpl.render(input) == "found");
  }

  SECTION("nonzero int")
  {
    fae::Template tmpl("$(if i5)found$(end)");

    REQUIRE(tmpl.render(input) == "found");
  }

  SECTION("empty string")
  {
    fae::Template tmpl("$(if sEmpty)found$(end)");

    REQUIRE(tmpl.render(input) == "found");
  }

  SECTION("full string")
  {
    fae::Template tmpl("$(if sFull)found$(end)");

    REQUIRE(tmpl.render(input) == "found");
  }
}

TEST_CASE("loops", "[template]")
{
  std::map<std::string, std::variant<std::array<int, 5>, std::vector<int>, std::deque<int>>> input;

  fae::Template tmpl("$(for n in collection)$(n)$(end)");

  SECTION("array")
  {
    input["collection"] = std::array<int, 5>{1, 2, 3, 4, 5};

    REQUIRE(tmpl.render(input) == "12345");
  }

  SECTION("vector")
  {
    input["collection"] = std::vector<int>{1, 2, 3, 4, 5};

    REQUIRE(tmpl.render(input) == "12345");
  }

  SECTION("deque")
  {
    input["collection"] = std::deque<int>{1, 2, 3, 4, 5};

    REQUIRE(tmpl.render(input) == "12345");
  }
}

TEST_CASE("invalid template", "[template]")
{
  REQUIRE_THROWS_AS([](){ fae::Template t("$()"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(if spaceAfter )"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(if word anotherWord)"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(not-a-valid-variable-name)"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(for n)"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(for n in)"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(for n in spaceAfter )"); }(), fae::FaeException);

  REQUIRE_THROWS_AS([](){ fae::Template t("$(for n in word anotherWord)"); }(), fae::FaeException);
}
