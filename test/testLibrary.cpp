/**
 * @file testLibrary.cpp
 * @brief Tests for fae::Library
 * @author Nick Muggio
 */

#include <filesystem>
#include <fstream>

#include "catch2/catch.hpp"

#include "fae.hpp"

TEST_CASE("library", "[library]")
{
  std::filesystem::path libDir = std::filesystem::temp_directory_path() / "fae_library_test_dir";
  std::filesystem::create_directory(libDir);

  std::ofstream stream(libDir / "t1.txt", std::ios::trunc);
  stream << "Hello, $(place)";
  stream.close();

  stream = std::ofstream(libDir / "t2.txt", std::ios::trunc);
  stream << "I'm $(invalid";
  stream.close();

  std::filesystem::create_directory(libDir / "nested");

  stream = std::ofstream(libDir / "nested" / "t3.txt", std::ios::trunc);
  stream << "Kaboom!";
  stream.close();

  stream = std::ofstream(libDir / "inc.txt", std::ios::trunc);
  stream << "$(include t1.txt) - $(include nested/t3.txt)";
  stream.close();

  fae::Input<std::string> input;
  input["place"] = "Mars";

  SECTION("recursive, ignore bad")
  {
    fae::Library l;
    REQUIRE_NOTHROW(l = fae::Library(libDir, true, true));

    REQUIRE(l.render("t1.txt", input) == "Hello, Mars");
    REQUIRE_THROWS_AS(l.render("t2.txt", input), fae::FaeException);
    REQUIRE(l.render("nested/t3.txt", input) == "Kaboom!");
    REQUIRE(l.render("inc.txt", input) == "Hello, Mars - Kaboom!");
  }

  SECTION("throw on bad")
  {
    fae::Library l;
    REQUIRE_THROWS_AS(l = fae::Library(libDir, true, false), fae::FaeException);
  }

  SECTION("non recursive, ignore bad")
  {
    fae::Library l;
    REQUIRE_NOTHROW(l = fae::Library(libDir, false, true));

    REQUIRE(l.render("t1.txt", input) == "Hello, Mars");
    REQUIRE_THROWS_AS(l.render("t2.txt", input), fae::FaeException);
    REQUIRE_THROWS_AS(l.render("nested/t3.txt", input), fae::FaeException);
    REQUIRE(l.render("inc.txt", input) == "Hello, Mars - ");
  }
}
