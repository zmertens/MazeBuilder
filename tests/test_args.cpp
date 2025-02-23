#include <string>
#include <sstream>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>

using namespace std;
using namespace mazes;

TEST_CASE("Args method parses correctly", "[parses]") {
    args args_handler{};

    SECTION("Help requested") {
        vector<string> args_vec = { "app", "-h" };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE(args_handler.get("h").empty());
    }

    SECTION("Version requested") {
        vector<string> args_vec = { "app", "-v" };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE(args_handler.get("v").empty());
    }

    SECTION("Help and version requested") {
        vector<string> args_vec = { "app", "-hv" };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE(args_handler.get("hv").empty());
    }

    SECTION("Help and version requested") {
        vector<string> args_vec = { "app", "-vh" };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE(args_handler.get("vh").empty());
    }

    //SECTION("Invalid argument") {
    //    vector<string> args_vec = { "app", "-invalid" };
    //    REQUIRE_THROWS_AS(args_handler.parse(args_vec), std::invalid_argument);
    //}

    SECTION("Valid short arguments") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-d" };
        REQUIRE_NOTHROW(args_handler.parse(args_vec));
    }
}

TEST_CASE("Args method dumps output correctly", "[dumps]") {
    args args_handler{};
    SECTION("Dump empty args") {
        REQUIRE(args_handler.dump_s().empty());
    }
    SECTION("Dump args") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-d" };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE_FALSE(args_handler.dump_s().empty());
    }
}
