#include <string>
#include <sstream>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/args.h>

using namespace std;
using namespace mazes;

TEST_CASE( "Args are built by vector", "[args]" ) {
    unsigned int seed = 32u;
    unsigned int columns = 1'001u;
    unsigned int height = 11u;
    unsigned int rows = 1'002u;

    string output = "maze.obj";
    string help_message = "My Maze Builder Program\n";
    string version_message = "0.0.1\n";
    bool interactive = true;
    string algorithm = "sidewinder";

    static const vector<string> LONG_ARGS = {
        "maze_builder.exe",
        "--seed=" + to_string(seed),
        "--algorithm=" + algorithm,
        "--output=" + output,
        "--columns=" + to_string(columns),
        "--rows=" + to_string(rows),
        "--height=" + to_string(height),
        "--distances"
    };

    REQUIRE(LONG_ARGS.empty() == false);

    mazes::args args{};
    REQUIRE(true == args.parse(LONG_ARGS));

    REQUIRE_FALSE(args.help.empty());
    REQUIRE_FALSE(args.version.empty());
    REQUIRE(args.algo.compare(algorithm) == 0);
    REQUIRE(args.seed == seed);
    REQUIRE(args.output.compare(output) == 0);
    REQUIRE(args.columns == columns);
    REQUIRE(args.height == height);
    REQUIRE(args.rows == rows);
	REQUIRE(args.distances == true);

    // Check the ostream operator
    stringstream ss;
    ss << args;
    REQUIRE(!ss.str().empty());

    static const vector<string> SHORT_ARGS = {
		"maze_builder.exe",
		"-s", to_string(seed),
		"-a", algorithm,
		"-o", output,
		"-c", to_string(columns),
		"-r", to_string(rows),
		"-y", to_string(height), 
        "-d"};
    // First-come-first-serve and grab 'interactive'
    mazes::args args2;
    REQUIRE(true == args2.parse(SHORT_ARGS));
	// Parsing help switch will break the loop
    REQUIRE(true == !args2.help.empty());
    REQUIRE(true == !args2.version.empty());
    REQUIRE(args2.distances == true);
}

TEST_CASE("Args are bad and cannot be built", "[args]") {
    static const vector<string> BAD_SHORT_ARGS = {
    "maze_builder.exe",
    "-x",
    "-y",
    "-z"
    };
    // First-come-first-serve and grab 'interactive'
    mazes::args args;
    REQUIRE(false == args.parse(BAD_SHORT_ARGS));
    REQUIRE_NOTHROW(args.parse(BAD_SHORT_ARGS));
}
