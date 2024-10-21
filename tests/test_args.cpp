#include <string>
#include <sstream>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/args_builder.h>

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

    mazes::args_builder builder{ cref(LONG_ARGS) };
    mazes::args maze_args = builder.build();

    REQUIRE(maze_args.help.empty() == true);
    REQUIRE(maze_args.version.empty() == true);
    REQUIRE(maze_args.interactive == false);
    REQUIRE(maze_args.algorithm.compare(algorithm) == 0);
    REQUIRE(maze_args.seed == seed);
    REQUIRE(maze_args.output.compare(output) == 0);
    REQUIRE(maze_args.columns == columns);
    REQUIRE(maze_args.height == height);
    REQUIRE(maze_args.rows == rows);
	REQUIRE(maze_args.distances == true);

    // Check the ostream operator
    stringstream ss;
    ss << maze_args;
    REQUIRE(!ss.str().empty());
;
    // Check help message
    auto&& maze_args_plus_help = builder.help(help_message).build();
    REQUIRE(maze_args_plus_help.help.compare(help_message) == 0);

    // Check version message
    builder.clear();
    auto&& maze_args_plus_version = builder.version(version_message).build();
    REQUIRE(maze_args_plus_version.version.compare(version_message) == 0);

    static const vector<string> SHORT_ARGS = {
		"maze_builder.exe",
		"-s", to_string(seed),
		"-i",
		"-a", algorithm,
		"-o", output,
		"-c", to_string(columns),
		"-r", to_string(rows),
		"-y", to_string(height), "-d"};
    // First-come-first-serve and grab 'interactive'
    mazes::args_builder builder2{ cref(SHORT_ARGS) };
    auto&& maze_args2 = builder2.build();
    REQUIRE(maze_args2.interactive == true);
	// Parsing help switch will break the loop
    REQUIRE(maze_args2.help.empty());
    REQUIRE(maze_args2.version.empty());
    REQUIRE(maze_args2.distances == true);
}

TEST_CASE("Args are bad and cannot be built", "[args]") {
    static const vector<string> BAD_SHORT_ARGS = {
    "maze_builder.exe",
    "-x",
    "-y",
    "-z"
    };
    // First-come-first-serve and grab 'interactive'
    mazes::args_builder builder2{ cref(BAD_SHORT_ARGS) };
    auto&& maze_args2 = builder2.build();
    REQUIRE_NOTHROW(maze_args2);
}

TEST_CASE("Args has help and version", "[help, version]") {
    static const vector<string> SHORT_ARGS = {
    "maze_builder.exe",
    "-h", "-v",
    };
    // First-come-first-serve and grab 'interactive'
    mazes::args_builder builder2{ cref(SHORT_ARGS) };
    auto&& maze_args2 = builder2.build();
    // Parsing help switch will break the loop
    REQUIRE(!maze_args2.help.empty());
    REQUIRE(maze_args2.version.empty());
}
