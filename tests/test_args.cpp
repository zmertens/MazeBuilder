#include <string>
#include <functional>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>

using namespace std;
using namespace mazes;

TEST_CASE("Args parses correctly", "[good parses]") {
    args args_handler{};

    SECTION("Help requested") {
        vector<string> args_vec = { "-h", "--help   " };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE(!args_handler.get().empty());
    }

    SECTION("Version requested") {
        vector<string> args_vec = { "  --version", " -v" };
        REQUIRE(args_handler.parse(args_vec));
        REQUIRE(!args_handler.get().empty());
    }

    SECTION("Help and version requested") {
        vector<string> args_vec = { "-hv" };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Short arguments 1") {
        vector<string> args_vec = { "-s500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "500");
    }

    SECTION("Short arguments 2") {
        vector<string> args_vec = { "-s 500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "500");
    }

    SECTION("Short arguments 3") {
        vector<string> args_vec = { "-r 10", "-s 500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "500");
        itr = args_handler.get().find("-r");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "10");
    }
    
    SECTION("Mixed arguments 1") {
        string valid_mixed_args = "--rows=10 --columns=10 -s2 --algo=binary_tree --output=1.txt --distances";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
        REQUIRE(args_handler.get("--rows") == "10");
        REQUIRE(args_handler.get("--columns") == "10");
        REQUIRE(args_handler.get("-s") == "2");
        REQUIRE(args_handler.get("--distances").empty());
        REQUIRE(args_handler.get("--output") == "1.txt");
        REQUIRE(args_handler.get("--algo") == "binary_tree");
    }

    SECTION("Mixed arguments 2") {
        string valid_mixed_args = "--rows=10 --columns=10 -s2 --algo=binary_tree --output=1.txt -d";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
        REQUIRE(args_handler.get("--rows") == "10");
        REQUIRE(args_handler.get("--columns") == "10");
        REQUIRE(args_handler.get("-s") == "2");
        REQUIRE(args_handler.get("-d").empty());
        REQUIRE(args_handler.get("--output") == "1.txt");
        REQUIRE(args_handler.get("--algo") == "binary_tree");
    }

    SECTION("No args") {
        string none = "";
        REQUIRE(args_handler.parse(cref(none)));
    }

    SECTION("No args 2") {
        vector<string> args_vec = { "" };
        REQUIRE(args_handler.parse(cref(args_vec)));
    }
}

TEST_CASE("Args method fails parse", "[fails parse]") {
    args args_handler{};

    SECTION("Repeated arguments") {
        string valid_mixed_args = "--rows=10 -r 10 --rows=11";
        REQUIRE_FALSE(args_handler.parse(cref(valid_mixed_args)));
    }

    SECTION("Bad long arguments 1") {
        string bad_long_args = "--rows 10 --columns 10 --seed 2 --distances 1 --output stdout";
        REQUIRE_FALSE(args_handler.parse(cref(bad_long_args)));
    }

    SECTION("Short arguments 1") {
        vector<string> args_vec = { "-r 10", "-c 10", "-s 2", "-d 1", "-o stdout" };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
    }

    SECTION("Short arguments 2") {
        string args = "-r 10 -c 10 -s 2 -d 1 -o stdout";
        REQUIRE_FALSE(args_handler.parse(cref(args)));
    }

    SECTION("Short arguments 3") {
        vector<string> args_vec = { "app", "r", "10", "c", "10", "s", "2", "d", "h" };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
    }
    SECTION("Mixed arguments") {
        string invalid_mixed_args = "columns s3 app";
        REQUIRE_FALSE(args_handler.parse(cref(invalid_mixed_args)));
    }
    SECTION("Just bad arguments") {
        vector<string> args_vec = { "--thing", "= ? ? ? " };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
    }
    SECTION("More bad arguments") {
        string pi = "\u03C0";
        string lambda = u8"\u03BB";
        string pi_lambda = pi + lambda;
        REQUIRE_FALSE(args_handler.parse(cref(pi_lambda)));
    }
}

TEST_CASE("Args method prints correctly", "[prints]") {
    args args_handler{};
    SECTION("Print empty args") {
        auto s = args::to_str(args_handler);
        REQUIRE(s.empty());
    }
    SECTION("Print args") {
        vector<string> args_vec = { "-r 10", "-c10", "-s 2", "-d" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto s = args::to_str(args_handler);
        REQUIRE_FALSE(s.empty());
    }
}

TEST_CASE("Args can handle a JSON input string", "[json input string]") {
    args args_handler{};
    SECTION("JSON input 1") {
        string valid_json = R"json(-j `{
            "rows": 10,
            "columns": 10,
            "seed": 2,
            "distances": true,
            "output": "1.txt"
        }`)json";
        REQUIRE(args_handler.parse(cref(valid_json)));

        const auto& m = args_handler.get();
        REQUIRE(m.find("rows") != m.end());
        REQUIRE(m.find("columns") != m.end());
        REQUIRE(m.find("seed") != m.end());
        REQUIRE(m.find("distances") != m.end());
        REQUIRE(m.find("output") != m.end());
    }

    SECTION("JSON input 2") {
        string valid_json = R"json(--json=`{
            "c": 10,
            "s": 2,
            "r": 10,
            "d": false,
            "o": "1.txt"
        }`)json";
        REQUIRE(args_handler.parse(cref(valid_json)));

        const auto& m = args_handler.get();
        REQUIRE(m.find("r") != m.end());
        REQUIRE(m.find("c") != m.end());
        REQUIRE(m.find("s") != m.end());
        REQUIRE(m.find("d") != m.end());
        REQUIRE(m.find("o") != m.end());
    }

    SECTION("Syntax error in JSON input") {
        // Invalid because there's no closing bracket
        string invalid_json = R"json(-j `{
            "rows": 10,
            "columns": 10,
            "seed": 2,
            "distances": true,
            "output": "1.txt"
        )json";
        REQUIRE_FALSE(args_handler.parse(cref(invalid_json)));
    }
}

TEST_CASE("Args can handle a JSON input file", "[json input file]") {
    args args_handler{};
    SECTION("JSON input file") {
        string json_file_valid = " -j maze_dfs.json  ";
        REQUIRE(args_handler.parse(cref(json_file_valid)));

        const auto& m = args_handler.get();
        REQUIRE(m.find("rows") != m.end());
        REQUIRE(m.find("columns") != m.end());
        REQUIRE(m.find("seed") != m.end());
        REQUIRE(m.find("distances") != m.end());
        REQUIRE(m.find("output") != m.end());

        args_handler.clear();

        json_file_valid = " --json=maze_dfs.json  ";
        REQUIRE(args_handler.parse(cref(json_file_valid)));
        REQUIRE_FALSE(args_handler.get("--json").empty());
    }
}
