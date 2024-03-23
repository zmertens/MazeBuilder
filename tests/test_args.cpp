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
        {"version", ""},
        { "state", "2" }
    };

    mazes::args_builder _args_builder {args};
    auto&& args_built {_args_builder.build()};

    REQUIRE(args == args_built);

    auto&& state = _args_builder.get_state();
    REQUIRE(state == args_state::READY_TO_ROCK);
    REQUIRE(args.at("algorithm").compare(args_built.at("algorithm")) == 0);
    REQUIRE(_args_builder.get_seed() == 0);
    REQUIRE(_args_builder.is_interactive() == false);
    REQUIRE(_args_builder.get_algorithm().compare("sidewinder") == 0);
    REQUIRE(_args_builder.get_output().compare("stdout") == 0);
    REQUIRE(_args_builder.get_width() == 100);
    REQUIRE(_args_builder.get_height() == 10);
    REQUIRE(_args_builder.get_length() == 100);

    stringstream ss;
    ss << _args_builder;
    REQUIRE(!ss.str().empty());
}

TEST_CASE("Just needs help", "[help]" ) {
    // it is up to the caller to provide program name with arguments
    vector<string> args_vec {"maze_builder"};
    static constexpr auto version {"1.0.0"};
    static constexpr auto help {"TESTING HELP MESSAGE!!"};

    SECTION("Testing short args") {
        args_vec.emplace_back("-h");
        mazes::args_builder _args_builder {version, help, args_vec};
        // build with parse the arguments
        auto&& args = _args_builder.build();
        auto&& state = _args_builder.get_state();
        REQUIRE(state == args_state::JUST_NEEDS_HELP);
        REQUIRE(!args.empty());
    }
    SECTION("Testing long args") {
        args_vec.emplace_back("--help");
        mazes::args_builder _args_builder {version, help, args_vec};
        // build with parse the arguments
        auto&& args = _args_builder.build();
        auto&& state = _args_builder.get_state();
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
        mazes::args_builder _args_builder {version, help, args_vec};
        auto&& args = _args_builder.build();
        REQUIRE(!args.empty());
        REQUIRE(_args_builder.get_seed() == 42);
        REQUIRE(_args_builder.get_width() == 101);
        REQUIRE(_args_builder.get_length() == 50);
        REQUIRE(_args_builder.get_height() == 8);
        REQUIRE(_args_builder.get_algorithm().compare("sidewinder") == 0);
        REQUIRE(_args_builder.get_output().compare("maze.obj") == 0);
    }
    SECTION("Throwing exception") {
        args_vec.clear();
        args_vec.emplace_back("maze_builder2");
        args_vec.emplace_back("--blah=binary_blah");
        args_builder args_builder {version, help, args_vec};
        REQUIRE_THROWS(args_builder.build());
    }
}

