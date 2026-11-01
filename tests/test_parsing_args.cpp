#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>
#include <vector>

#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/parsing_state.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>

#include "parsing_fixture.h"

using namespace mazes;
using namespace std;

static constexpr auto ARRAY_DOT_JSON_FILE{"array.json"};
static constexpr auto MAZE_DOT_JSON_FILE{"maze.json"};

TEST_CASE("Args static checks", "[args_static_checks]")
{
    STATIC_REQUIRE(std::is_default_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::args>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::args>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::json_helper>::value);
}

TEST_CASE("Args stores raw input", "[args_raw]")
{
    args a{};

    SECTION("Empty inputs do not parse")
    {
        REQUIRE_FALSE(a.parse(vector<string>{}));
        REQUIRE_FALSE(a.parse(string{}));
        REQUIRE_FALSE(a.parse(0, nullptr));
        REQUIRE(a.count() == 0);
    }

    SECTION("Tokens are joined into one raw map")
    {
        REQUIRE(a.parse(vector<string>{"-a", "dfs", "-r", "5"}));
        REQUIRE(a.count() == 1);
        REQUIRE(a.get(args::RAW_WORD_STR).value() == "-a dfs -r 5");
    }

    SECTION("Program name is dropped when declared")
    {
        REQUIRE(a.parse(vector<string>{"./maze", "-a", "dfs"}, true));
        REQUIRE(a.get(args::RAW_WORD_STR).value() == "-a dfs");
    }

    SECTION("Program name only leaves nothing to parse")
    {
        REQUIRE_FALSE(a.parse(vector<string>{"./maze"}, true));
    }

    SECTION("String input is stored as raw")
    {
        REQUIRE(a.parse(string{"-r 3 -c 4"}));
        REQUIRE(a.get(args::RAW_WORD_STR).has_value());
    }
}

TEST_CASE("Args set, front, pop_front and clear", "[args_accessors]")
{
    args a{};
    a.set({args::arg_map{{"rows", "1"}}, args::arg_map{{"rows", "2"}}});

    REQUIRE(a.count() == 2);
    REQUIRE(a.front().at("rows") == "1");
    REQUIRE(a.get("rows").value() == "1");
    REQUIRE_FALSE(a.get("missing").has_value());

    REQUIRE(a.pop_front());
    REQUIRE(a.front().at("rows") == "2");

    a.clear();
    REQUIRE(a.count() == 0);
    REQUIRE(a.front().empty());
    REQUIRE_FALSE(a.pop_front());
}

TEST_CASE("Args copies are independent", "[args_copy]")
{
    args a{};
    a.set({args::arg_map{{"rows", "1"}}});

    args b{a};
    a.clear();
    REQUIRE(b.count() == 1);
    REQUIRE(b.get("rows").value() == "1");
}

TEST_CASE("parsing_state parses short, long and inline forms", "[parsing_state]")
{
    test_support::parsing_fixture f;

    SECTION("Short flags")
    {
        REQUIRE(f.run("-a dfs -r 5 -c 6 -l 2 -s 7 -o stdout"));
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::ALGO_ID_WORD_STR) == "dfs");
        REQUIRE(m.at(args::ROW_WORD_STR) == "5");
        REQUIRE(m.at(args::COLUMN_WORD_STR) == "6");
        REQUIRE(m.at(args::LEVEL_WORD_STR) == "2");
        REQUIRE(m.at(args::SEED_WORD_STR) == "7");
        REQUIRE(m.at(args::OUTPUT_ID_WORD_STR) == "stdout");
    }

    SECTION("Long options and bare words")
    {
        REQUIRE(f.run("--algo prims rows 3 --columns 4"));
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::ALGO_ID_WORD_STR) == "prims");
        REQUIRE(m.at(args::ROW_WORD_STR) == "3");
        REQUIRE(m.at(args::COLUMN_WORD_STR) == "4");
    }

    SECTION("Inline --opt=value")
    {
        REQUIRE(f.run("--algo=sidewinder --rows=8"));
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::ALGO_ID_WORD_STR) == "sidewinder");
        REQUIRE(m.at(args::ROW_WORD_STR) == "8");
    }

    SECTION("Extra whitespace is collapsed")
    {
        REQUIRE(f.run("  -r   2 \t -c\n3  "));
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::ROW_WORD_STR) == "2");
        REQUIRE(m.at(args::COLUMN_WORD_STR) == "3");
    }

    SECTION("Repeated keys: last value wins")
    {
        REQUIRE(f.run("-r 2 --rows 2 -r 3"));
        REQUIRE(f.parsed().front().at(args::ROW_WORD_STR) == "3");
    }

    SECTION("Bare distances flag means true")
    {
        REQUIRE(f.run("-r 3 -d"));
        REQUIRE(f.parsed().front().at(args::DISTANCES_WORD_STR) == args::TRUE_VALUE);
    }

    SECTION("Distances accepts a slice")
    {
        REQUIRE(f.run("-d [0:10]"));
        REQUIRE(f.parsed().front().at(args::DISTANCES_WORD_STR) == "[0:10]");
    }

    SECTION("Raw input is consumed")
    {
        REQUIRE(f.run("-r 3"));
        REQUIRE(f.raw().count() == 0);
    }

    SECTION("Parsed args are kept when already present")
    {
        REQUIRE(f.run("-r 3"));
        REQUIRE(f.run("-r 9"));
        REQUIRE(f.parsed().front().at(args::ROW_WORD_STR) == "3");
    }
}

TEST_CASE("parsing_state handles quoted values", "[parsing_state_quotes]")
{
    test_support::parsing_fixture f;

    SECTION("Double quotes keep spaces and are stripped")
    {
        REQUIRE(f.run("-o \"my maze.txt\" -r 3"));
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::OUTPUT_ID_WORD_STR) == "my maze.txt");
        REQUIRE(m.at(args::ROW_WORD_STR) == "3");
    }
}

TEST_CASE("parsing_state handles JSON", "[parsing_state_json]")
{
    test_support::parsing_fixture f;

    SECTION("JSON string via flag")
    {
        REQUIRE(f.run("-j `{ \"rows\": 10, \"columns\": 12, \"seed\": 2, \"algo\": \"dfs\", \"distances\": true }`"));
        REQUIRE(f.parsed().count() == 1);
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::ROW_WORD_STR) == "10");
        REQUIRE(m.at(args::COLUMN_WORD_STR) == "12");
        REQUIRE(m.at(args::SEED_WORD_STR) == "2");
        REQUIRE(m.at(args::ALGO_ID_WORD_STR) == "dfs");
        REQUIRE(m.contains(args::DISTANCES_WORD_STR));
        REQUIRE_FALSE(m.at(args::JSON_FLAG_STR).empty());
        REQUIRE_FALSE(m.at(args::JSON_OPTION_STR).empty());
        REQUIRE_FALSE(m.at(args::JSON_WORD_STR).empty());
    }

    SECTION("JSON string via --json=")
    {
        REQUIRE(f.run("--json=`{\n\"rows\": 4,\n\"columns\": 5\n}`"));
        const auto m = f.parsed().front();
        REQUIRE(m.at(args::ROW_WORD_STR) == "4");
        REQUIRE(m.at(args::COLUMN_WORD_STR) == "5");
    }

    SECTION("JSON file with one object")
    {
        REQUIRE(f.run(string{"--json="} + MAZE_DOT_JSON_FILE));
        REQUIRE(f.parsed().count() == 1);
        REQUIRE(f.parsed().front().at(args::SEED_WORD_STR) == "9001");
    }

    SECTION("JSON array yields one map per object")
    {
        REQUIRE(f.run(string{"--json="} + ARRAY_DOT_JSON_FILE));
        REQUIRE(f.parsed().count() > 1);
    }

    SECTION("Malformed JSON is rejected")
    {
        REQUIRE_FALSE(f.run("-j `{ \"columns\": 10 \"seed\": 2 }`"));
        REQUIRE(f.parsed().count() == 0);
    }

    SECTION("Non-numeric JSON values are rejected")
    {
        REQUIRE_FALSE(f.run("-j `{ \"rows\": \"abc\" }`"));
        REQUIRE(f.parsed().count() == 0);
    }
}

TEST_CASE("parsing_state rejects bad input", "[parsing_state_invalid]")
{
    test_support::parsing_fixture f;

    SECTION("Empty and blank input")
    {
        REQUIRE_FALSE(f.run(""));
        REQUIRE_FALSE(f.run("   "));
    }

    SECTION("Unknown options")
    {
        REQUIRE_FALSE(f.run("-r 5 --bogus"));
        REQUIRE_FALSE(f.run("-z 5"));
        REQUIRE_FALSE(f.run("maze_builder"));
    }

    SECTION("Missing values")
    {
        REQUIRE_FALSE(f.run("-r"));
        REQUIRE_FALSE(f.run("-r -c 5"));
        REQUIRE_FALSE(f.run("-a"));
    }

    SECTION("Non-numeric values")
    {
        REQUIRE_FALSE(f.run("-r abc"));
        REQUIRE_FALSE(f.run("-s -5"));
        REQUIRE_FALSE(f.run("--columns=1x"));
    }

    SECTION("Failures leave no partial maps and consume raw")
    {
        REQUIRE_FALSE(f.run("-r 5 --bogus"));
        REQUIRE(f.parsed().count() == 0);
        REQUIRE(f.raw().count() == 0);
    }
}
