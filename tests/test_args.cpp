#include <string>
#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>

using namespace std;
using namespace mazes;

TEST_CASE("Args parses correctly", "[parses]") {
    args args_handler{};

    SECTION("Help requested") {
        vector<string> args_vec = { "app", "-h" };
        bool needs_help{ false };
        //args_handler.add_flag("-h", ref(needs_help), "Needs help");
        args_handler.add_option("-r", "Rows");
        REQUIRE(args_handler.parse(args_vec));
        //REQUIRE(needs_help);
    }

    //SECTION("Help requested 2") {
    //    vector<string> args_vec = { "  app  ", "--help  " };
    //    REQUIRE(args_handler.parse(args_vec));
    //}

    //SECTION("Version requested") {
    //    vector<string> args_vec = { "app", " -v" };
    //    REQUIRE(args_handler.parse(args_vec));
    //}

    //SECTION("Version requested 2") {
    //    vector<string> args_vec = { "app", "--version" };
    //    REQUIRE(args_handler.parse(args_vec));
    //}

    //SECTION("Help and version requested") {
    //    vector<string> args_vec = { "app", "-hv" };
    //    REQUIRE(args_handler.parse(args_vec));
    //}

    SECTION("Valid short arguments 1") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-d", "-ostdout"};
        REQUIRE(args_handler.parse(cref(args_vec)));
    }

    //SECTION("Valid short arguments 2") {
    //    vector<string> args_vec = { "app", "-s500" };
    //    REQUIRE(args_handler.parse(cref(args_vec)));
    //}

    //SECTION("Valid mixed arguments") {
    //    string valid_mixed_args = "app --rows=10 --columns=10 -s2 --algo=binary_tree --output=1.txt --distances";
    //    REQUIRE(args_handler.parse(cref(valid_mixed_args)));
    //}

    //SECTION("Valid repeated arguments") {
    //    string valid_mixed_args = "app --rows=10 -r 10 --rows=11";
    //    REQUIRE(args_handler.parse(cref(valid_mixed_args)));
    //}

    //SECTION("No arguments") {
    //    string none = "";
    //    REQUIRE(args_handler.parse(cref(none)));
    //}
}

//TEST_CASE("Args bad parse", "[bad parse]") {
//    args args_handler{};
//
//    SECTION("No args") {
//        vector<string> args_vec = { "" };
//        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
//    }
//
//    SECTION("Invalid short arguments") {
//        vector<string> args_vec = { "app", "r", "10", "c", "10", "s", "2", "d", "h" };
//        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
//    }
//
//    SECTION("Invalid mixed arguments") {
//        string invalid_mixed_args = "rows columns s3 app";
//        REQUIRE_FALSE(args_handler.parse(cref(invalid_mixed_args)));
//    }
//
//    SECTION("Invalid short arguments 2") {
//        vector<string> args_vec = { "app", "10", "-r", "-c", "", "-sd", "3", "-d", "-d" };
//        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
//    }
//
//    SECTION("Invalid short arguments 3") {
//        vector<string> args_vec = { "./app -s 500 -s 400" };
//        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
//    }
//
//    SECTION("Invalid long arguments") {
//        string invalid_long_args = "--roows= --columns=";
//        REQUIRE_FALSE(args_handler.parse(cref(invalid_long_args)));
//    }
//}

TEST_CASE("Args method prints correctly", "[prints]") {
    args args_handler{};

    SECTION("Print args") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-d" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        stringstream ss;
        ss << args_handler;
        REQUIRE_FALSE(ss.str().empty());
    }
}

//TEST_CASE("Args can handle a JSON input string", "[json input string]") {
//    args args_handler{};
//    SECTION("Valid JSON input") {
//        string valid_json = R"json(./app -j `{
//            "rows": 10,
//            "columns": 10,
//            "seed": 2,
//            "distances": true,
//            "output": "1.txt"
//        }`)json";
//        REQUIRE(args_handler.parse(cref(valid_json)));
//    }
//
//    SECTION("Valid JSON input 2") {
//        string valid_json = R"json(app -j  `   {
//            "rows": 10,
//            "columns": 10,
//            "seed": 2,
//            "distances": true,
//            "output": "1.txt"
//        }      `              )json";
//        REQUIRE(args_handler.parse(cref(valid_json)));
//    }
//
//    SECTION("Invalid JSON input") {
//        // Invalid because there's no closing bracket
//        string invalid_json = R"json(./app -j `{
//            "rows": 10,
//            "columns": 10,
//            "seed": 2,
//            "distances": true,
//            "output": "1.txt"
//        )json";
//        REQUIRE_FALSE(args_handler.parse(cref(invalid_json)));
//    }
//}
//
//TEST_CASE("Args can handle a JSON input file", "[json input file]") {
//    args args_handler{};
//    SECTION("Valid JSON input from string args") {
//        string json_file_valid = "  ./app -j maze_dfs.json  ";
//        REQUIRE(args_handler.parse(cref(json_file_valid)));
//    }
//
//    SECTION("Valid JSON input from file args 1") {
//        string json_file_valid = "./app -j maze_dfs.json";
//        REQUIRE(args_handler.parse(cref(json_file_valid)));
//    }
//
//    SECTION("Valid JSON input from file args 2") {
//        string json_file_valid = "./app --json maze_dfs.json";
//        REQUIRE(args_handler.parse(cref(json_file_valid)));
//    }
//
//    SECTION("Valid JSON input from file args 3") {
//        string json_file_valid = "./app --json=maze_dfs.json";
//        REQUIRE(args_handler.parse(cref(json_file_valid)));
//    }
//
//    SECTION("Valid JSON input from file args 4") {
//        vector<string> args_vec = { "./app", "-j", "maze_dfs.json" };
//        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
//    }
//}
