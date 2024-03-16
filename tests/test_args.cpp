#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include "../sources/args_builder.h"

using namespace std;
using namespace mazes;

TEST_CASE( "Args are computed", "[args]" ) {
    std::unordered_map<std::string, std::string> args = {
        {"algorithm", "sidewinder"},
        {"seed", "0"},
        {"interactive", "0"},
        {"output", "stdout"},
        {"width", "100"},
        {"length", "100"},
        {"height", "10"},
        {"help", ""},
        {"version", ""}
    };

    args_builder args_builder {args};
    auto&& args_built {args_builder.build()};

    REQUIRE(args == args_built);

    auto&& state = args_builder.get_state();
    REQUIRE(state == args_state::READY_TO_ROCK);
    REQUIRE(args.at("algorithm").compare(args_built.at("algorithm")) == 0);
    REQUIRE(args_builder.get_seed() == 0);
    REQUIRE(args_builder.is_interactive() == false);
    REQUIRE(args_builder.get_algo().compare("sidewinder") == 0);
    REQUIRE(args_builder.get_output().compare("stdout") == 0);
    REQUIRE(args_builder.get_width() == 100);
    REQUIRE(args_builder.get_height() == 10);
    REQUIRE(args_builder.get_length() == 100);

    stringstream ss;
    ss << args_builder;
    REQUIRE(!ss.str().empty());
}

TEST_CASE("Just needs help", "[help]" ) {
    static char* argv[] = {"maze_builder", "-h"};
    static constexpr auto argc {2};
    static constexpr auto version {"1.0.0"};
    static constexpr auto help {"TESTING HELP MESSAGE!!"};
    args_builder args_builder {version, help, argc, argv};

    auto&& state = args_builder.get_state();
    REQUIRE(state == args_state::JUST_NEEDS_HELP);
}

TEST_CASE("Args can be parsed", "[parsing args]" ) {
    static char* argv[] = {"maze_builder", "--algorithm=sidewinder", "-s", "42", "-w", "101", "-l", "50", "--output=maze.obj"};
    static constexpr auto argc {9};
    static constexpr auto version {"1.0.0"};
    static constexpr auto help {"TESTING HELP MESSAGE!!"};
    args_builder args_builder {version, help, argc, argv};

    REQUIRE(args_builder.get_seed() == 42);
    REQUIRE(args_builder.get_width() == 101);
    REQUIRE(args_builder.get_length() == 50);
    REQUIRE(args_builder.get_algo().compare("sidewinder") == 0);
    // SECTION("Throwing exception") {
    //     auto&& args = args_builder.build();
    //     args["width"] = "";
    //     REQUIRE_THROWS(args_builder.get_width());
    // }
}

