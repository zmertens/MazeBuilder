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
        vector<string> args_vec = { "-s", "500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "500");
    }

    SECTION("Short arguments 2") {
        vector<string> args_vec = { "-r", "10", "-s", "500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "500");
        itr = args_handler.get().find("-r");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "10");
    }

    SECTION("Short arguments 3") {
        string args = "-r 10 -c 10 -s 2 -d 1 -o stdout";
        REQUIRE(args_handler.parse(cref(args)));
    }

    SECTION("Long arguments with no equals sign") {
        string long_args_no_equals_sign = "--rows 10 --columns 10 --seed 2 --distances 1 --output stdout";
        REQUIRE(args_handler.parse(cref(long_args_no_equals_sign)));
    }
    
    SECTION("Mixed arguments 1") {
        string valid_mixed_args = "--rows=10 --columns=10 -s 2 --algo=binary_tree --output=1.txt --distances";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
        REQUIRE(args_handler.get("--rows").has_value());
        REQUIRE(args_handler.get("--columns").has_value());
        REQUIRE(args_handler.get("-s").has_value());
        REQUIRE(args_handler.get("--distances").has_value());
        REQUIRE(args_handler.get("--output").has_value());
        REQUIRE(args_handler.get("--algo").has_value());
        auto algo_val = args_handler.get("--algo");
        REQUIRE(algo_val == "binary_tree");
    }

    SECTION("Mixed arguments 2") {
        string valid_mixed_args = "--rows=10 --columns=10 -s 2 --algo=binary_tree --output=1.txt -d";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
        REQUIRE(args_handler.get("-d").has_value());
    }

    SECTION("Mixed arguments 3") {
        vector<string> args_vec = { "-r", "10", "--columns", "15",  "-s", "500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "500");
        itr = args_handler.get().find("-r");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "10");
        itr = args_handler.get().find("--columns");
        REQUIRE(itr != args_handler.get().cend());
        REQUIRE(itr->second == "15");
    }

    SECTION("No args") {
        string none = "";
        REQUIRE(args_handler.parse(cref(none)));
    }

    SECTION("No args 2") {
        vector<string> args_vec = { "" };
        REQUIRE(args_handler.parse(cref(args_vec)));
    }

    SECTION("Repeated arguments are OK") {
        string valid_mixed_args = "--rows=10 -r 10 --rows=11";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
    }
}

TEST_CASE("Args method fails parse", "[fails parse]") {
    args args_handler{};

    SECTION("Short arguments 1") {
        vector<string> args_vec = { "-r 10", "-c 10", "-s 2", "-d 1", "-o stdout" };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
    }

    SECTION("Short arguments 3") {
        vector<string> args_vec = { "app", "r", "10", "c", "10", "s", "2", "d", "h" };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
    }

    SECTION("Short arguments 4") {
        vector<string> args_vec = { "-s500" };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
        auto itr = args_handler.get().find("-s");
        REQUIRE_FALSE(itr != args_handler.get().cend());
    }

    SECTION("Mixed arguments") {
        string invalid_mixed_args = "columns s3 app";
        REQUIRE_FALSE(args_handler.parse(cref(invalid_mixed_args)));
    }
    SECTION("Just bad arguments") {
        vector<string> args_vec = { "--thing = ? ? ?" };
        REQUIRE_FALSE(args_handler.parse(cref(args_vec)));
    }
    SECTION("Unicode as bad arguments") {
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
        vector<string> args_vec = { "-r", "10", "-c", "10", "-s", "2", "-d" };
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
        REQUIRE(args_handler.get("--json").has_value());
    }
}
