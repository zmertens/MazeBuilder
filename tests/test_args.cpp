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
    // it is up to the caller to provide a full list of args
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
    // it is up to the caller to provide program name with arguments
    vector<string> args_vec {"maze_builder"};
    static constexpr auto version {"1.0.0"};
    static constexpr auto help {"TESTING HELP MESSAGE!!"};

    SECTION("Testing short args") {
        args_vec.emplace_back("-h");
        args_builder args_builder {version, help, args_vec};
        // build with parse the arguments
        auto&& args = args_builder.build();
        auto&& state = args_builder.get_state();
        REQUIRE(state == args_state::JUST_NEEDS_HELP);
        REQUIRE(!args.empty());
    }
    SECTION("Testing long args") {
        args_vec.emplace_back("--help");
        args_builder args_builder {version, help, args_vec};
        // build with parse the arguments
        auto&& args = args_builder.build();
        auto&& state = args_builder.get_state();
        REQUIRE(state == args_state::JUST_NEEDS_HELP);
        REQUIRE(!args.empty());
    }
}

TEST_CASE("Args can be parsed", "[parsing args]" ) {
    vector<string> args_vec {"maze_builder", "--algorithm=sidewinder", "-s", "42", "-w", "101", "-l", "50", "-y", "8", "--output=maze.obj"};
    static constexpr auto version {"1.0.0"};
    static constexpr auto help {"TESTING HELP MESSAGE!!"};

    // build will parse the arguments
    SECTION("Business as usual args") {
        args_builder args_builder {version, help, args_vec};
        auto&& args = args_builder.build();
        REQUIRE(!args.empty());
        REQUIRE(args_builder.get_seed() == 42);
        REQUIRE(args_builder.get_width() == 101);
        REQUIRE(args_builder.get_length() == 50);
        REQUIRE(args_builder.get_height() == 8);
        REQUIRE(args_builder.get_algo().compare("sidewinder") == 0);
        REQUIRE(args_builder.get_output().compare("maze.obj") == 0);
    }
    // SECTION("Throwing exception") {
    //     args_vec.clear();
    //     args_vec.emplace_back("maze_builder2", "--blah=binary_blah");
    //     args_builder args_builder {version, help, args_vec};
    //     REQUIRE_THROWS(args_builder.build());
    // }
}

