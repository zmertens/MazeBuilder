#include <string>
#include <sstream>

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
        auto found = args_handler.get().find("hv") != args_handler.get().end();
        REQUIRE_FALSE(found);
    }

    SECTION("Valid short arguments") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-d" };
        REQUIRE(args_handler.parse(cref(args_vec)));
    }

    SECTION("Valid short arguments 2") {
        vector<string> args_vec = { "app", "-s500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
    }

    SECTION("Valid mixed arguments") {
        string valid_mixed_args = "app --rows=10 --columns=10 -s2 --algo=binary_tree --output=1.txt --distances";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
    }

    SECTION("Valid repeated arguments") {
        string valid_mixed_args = "app --rows=10 -r 10 --rows=11";
        REQUIRE(args_handler.parse(cref(valid_mixed_args)));
    }

    SECTION("No arguments") {
        string none = "";
        REQUIRE(args_handler.parse(cref(none)));
    }
}

TEST_CASE("Args method does not parse", "[no parse]") {
    args args_handler{};
    SECTION("Invalid short arguments") {
        vector<string> args_vec = { "app", "r", "10", "c", "10", "s", "2", "d", "h" };
        REQUIRE(args_handler.parse(cref(args_vec)));
    }
    SECTION("Invalid mixed arguments") {
        string invalid_mixed_args = "rows columns s3 app";
        REQUIRE(args_handler.parse(cref(invalid_mixed_args)));
        REQUIRE_FALSE(args_handler.get("app").empty());
    }
    SECTION("Invalid short arguments") {
        vector<string> args_vec = { "app", "10", "-r", "-c", "", "-sd", "3", "-d", "-d" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        REQUIRE(args_handler.get("r").empty());
        REQUIRE(args_handler.get("s").empty());
    }
    SECTION("Invalid long arguments") {
        string invalid_long_args = "--roows= --columns=";
        REQUIRE(args_handler.parse(cref(invalid_long_args)));
        REQUIRE(args_handler.get("roows").empty());
        REQUIRE(args_handler.get("columns").empty());
    }
}

TEST_CASE("Args method prints correctly", "[prints]") {
    args args_handler{};
    SECTION("Print empty args") {
        stringstream ss;
        ss << args_handler;
        REQUIRE(ss.str().empty());
    }
    SECTION("Print args") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-d" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        stringstream ss;
        ss << args_handler;
        REQUIRE_FALSE(ss.str().empty());
    }
}
