#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/string_view_utils.h>

using namespace mazes;
using namespace std;

TEST_CASE("Args static checks", "[args_static_checks]") {

    STATIC_REQUIRE(std::is_default_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::args>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::args>::value);
}

TEST_CASE("Args simple parses", "[simple_parses]") {

    args args_handler{};

    SECTION("Empty vector") {

        vector<string> args_vec;
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("App name only") {

        vector<string> args_vec = { "maze_builder" };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Help requested with vector of string") {

        vector<string> args_vec = { args::HELP_FLAG_STR, args::HELP_OPTION_STR, args::HELP_WORD_STR };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Version requested with vector of string") {

        vector<string> args_vec = { args::VERSION_FLAG_STR, args::VERSION_OPTION_STR, args::VERSION_WORD_STR };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Help and version requested short flags") {

        vector<string> args_vec = { args::VERSION_FLAG_STR, args::HELP_FLAG_STR };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Help and version requested short flags") {

        vector<string> args_vec_long = { args::HELP_OPTION_STR, args::VERSION_OPTION_STR };
        REQUIRE(args_handler.parse(args_vec_long));
    }
}

TEST_CASE("Args parses and can get values", "[parses_and_then_gets_value]") {

    static constexpr auto ALGO{ configurator::DEFAULT_ALGO_ID };
    static constexpr auto DISTANCES_START{ configurator::DEFAULT_DISTANCES_START };
    static constexpr auto DISTANCES_END{ configurator::DEFAULT_DISTANCES_END };
    static constexpr auto NUM_ROWS{ configurator::DEFAULT_ROWS };
    static constexpr auto NUM_COLS{ configurator::DEFAULT_COLUMNS };
    static constexpr auto OUTPUT{ configurator::DEFAULT_OUTPUT_ID };
    static constexpr auto SEED{ configurator::DEFAULT_SEED };

    static constexpr auto DEFAULT_FILE_NAME{ "maze.txt" };

    auto check_optional_equals_value = [](auto opt, auto val) -> bool {

        return opt.has_value() && opt.value() == val;
    };

    args args_handler{};

    SECTION("Parse and get rows value") {
        vector<string> args_vec = { args::ROW_FLAG_STR, to_string(NUM_ROWS) };
        REQUIRE(args_handler.parse(args_vec));
        
        // Test all forms of access for rows
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_FLAG_STR), to_string(NUM_ROWS)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_OPTION_STR), to_string(NUM_ROWS)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), to_string(NUM_ROWS)));
    }

    SECTION("Parse and get columns value") {
        vector<string> args_vec = { args::COLUMN_FLAG_STR, to_string(NUM_COLS)};
        REQUIRE(args_handler.parse(args_vec));
        
        // Test all forms of access for columns
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_FLAG_STR), to_string(NUM_COLS)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_OPTION_STR), to_string(NUM_COLS)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), to_string(NUM_COLS)));
    }

    SECTION("Parse and get seed value") {
        vector<string> args_vec = { args::SEED_FLAG_STR, to_string(SEED) };
        REQUIRE(args_handler.parse(args_vec));
        
        // Test all forms of access for seed
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_FLAG_STR), to_string(SEED)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_OPTION_STR), to_string(SEED)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), to_string(SEED)));
    }

    SECTION("Parse and get algorithm value") {
        vector<string> args_vec = { args::ALGO_ID_FLAG_STR, to_string_from_algo(ALGO) };
        REQUIRE(args_handler.parse(args_vec));
        
        // Test all forms of access for algorithm
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_FLAG_STR), to_string_from_algo(ALGO)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_OPTION_STR), to_string_from_algo(ALGO)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), to_string_from_algo(ALGO)));
    }

    SECTION("Parse and get output value") {
        vector<string> args_vec = { args::OUTPUT_ID_FLAG_STR, DEFAULT_FILE_NAME };
        REQUIRE(args_handler.parse(args_vec));

        // Test all forms of access for algorithm
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_FLAG_STR), DEFAULT_FILE_NAME));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_OPTION_STR), DEFAULT_FILE_NAME));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), DEFAULT_FILE_NAME));
    }

    SECTION("Parse and get distances value") {
        vector<string> args_vec = { args::DISTANCES_FLAG_STR };
        REQUIRE(args_handler.parse(args_vec));
        
        // Test all forms of access for distances flag
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), args::TRUE_VALUE));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), args::TRUE_VALUE));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("Parse and get distances value with slice notation") {

        // Explicit slice values
        static const string DISTANCES_SLICE_1 = args::DISTANCES_FLAG_STR + string("[") + to_string(DISTANCES_START) + ":" + to_string(DISTANCES_END) + "]";
        // Implicit slice starting point
        static const string DISTANCES_SLICE_2 = args::DISTANCES_OPTION_STR + string("=[") + ":" + to_string(DISTANCES_END) + "]";
        // Implicit slice ending point
        static const string DISTANCES_SLICE_3 = args::DISTANCES_FLAG_STR + string("[") + to_string(DISTANCES_START) + ":" + "]";

        REQUIRE(args_handler.parse(cref(DISTANCES_SLICE_1)));
        
        // Test all forms of access for distances with slice
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), DISTANCES_SLICE_1));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), DISTANCES_SLICE_1));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), DISTANCES_SLICE_1));
        
        // Test parsed slice values
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_START_STR), to_string(DISTANCES_START)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_END_STR), to_string(DISTANCES_END)));

        args_handler.clear();

        string rev_args = { DISTANCES_SLICE_1.crbegin(), DISTANCES_SLICE_1.crend() };
        REQUIRE_FALSE(args_handler.parse(cref(rev_args)));

        args_handler.clear();

        REQUIRE(args_handler.parse(cref(DISTANCES_SLICE_2)));

        // Test all forms of access for distances with slice
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), DISTANCES_SLICE_2));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), DISTANCES_SLICE_2));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), DISTANCES_SLICE_2));

        // Test parsed slice values
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_START_STR), to_string(DISTANCES_START)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_END_STR), to_string(DISTANCES_END)));

        args_handler.clear();

        REQUIRE(args_handler.parse(cref(DISTANCES_SLICE_3)));

        // Test all forms of access for distances with slice
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), DISTANCES_SLICE_3));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), DISTANCES_SLICE_3));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), DISTANCES_SLICE_3));

        // Test parsed slice values
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_START_STR), to_string(DISTANCES_START)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_END_STR), to_string(DISTANCES_END)));
    }
}

TEST_CASE("Args add options and flags", "[options_and_flags]") {

    args args_handler{};
    
    SECTION("Add new option") {
        // Test adding a new option that doesn't already exist
        REQUIRE(args_handler.add_option("-x,--extra", "Extra test option"));
        vector<string> args_vec = { "--extra", "test_value" };
        REQUIRE(args_handler.parse(args_vec));
        auto val = args_handler.get("--extra");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "test_value");
    }
    
    SECTION("Add new flag") {
        // Test adding a new flag that doesn't already exist
        REQUIRE(args_handler.add_flag("-f,--flag", "Test flag"));
        vector<string> args_vec = { "--flag" };
        REQUIRE(args_handler.parse(args_vec));
        auto val = args_handler.get("--flag");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "true");
    }
    
    SECTION("Test existing rows option") {
        // Test that the existing rows option works (already added in setup_cli)
        vector<string> args_vec = { "--rows", "15" };
        REQUIRE(args_handler.parse(args_vec));
        auto val = args_handler.get("--rows");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "15");
        // Also check alternate access methods
        REQUIRE(args_handler.get("-r").has_value());
        REQUIRE(args_handler.get("-r").value() == "15");
        REQUIRE(args_handler.get("rows").has_value());
        REQUIRE(args_handler.get("rows").value() == "15");
    }
    
    SECTION("Test existing distances flag") {
        // Test that the existing distances option works (already added in setup_cli)
        vector<string> args_vec = { "--distances" };
        REQUIRE(args_handler.parse(args_vec));
        auto val = args_handler.get("--distances");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "true");
        // Also check alternate access methods
        REQUIRE(args_handler.get("-d").has_value());
        REQUIRE(args_handler.get("-d").value() == "true");
        REQUIRE(args_handler.get("distances").has_value());
        REQUIRE(args_handler.get("distances").value() == "true");
    }
}

TEST_CASE("Args can handle a JSON string input", "[json_string_input]") {

    // Missing a comma
    static constexpr auto INVALID_JSON_STR_1 = R"json(`{
            "columns": 10,
            "seed": 2
            "rows": 10,
            "distances": false,
            "algo": "dfs",
            "output": "validjsonstr2.txt"
        }`)json";

    static constexpr auto VALID_JSON_STR_1 = R"json(`{
            "rows": 10,
            "columns": 10,
            "seed": 2,
            "distances": true,
            "output": "validjsonstr1.txt",
            "algo": "sidewinder"
        }`)json";

    static const string VALID_JSON_STR_2 = \
        "  `  {  \"r\": " + to_string(configurator::DEFAULT_ROWS) \
        + ", \n \"c\": " + to_string(configurator::DEFAULT_COLUMNS) \
        + "\n  }  `  ";

    args args_handler{};

    SECTION("Parse JSON string 1") {
        vector<string> args_vec = { args::JSON_FLAG_STR, VALID_JSON_STR_1 };
        REQUIRE(args_handler.parse(args_vec));

        const auto& m = args_handler.get();
        REQUIRE(m.has_value());

        const auto& m_val = m.value();
        REQUIRE_FALSE(m_val.at(args::JSON_FLAG_STR).empty());
        REQUIRE_FALSE(m_val.at(args::JSON_OPTION_STR).empty());
        REQUIRE_FALSE(m_val.at(args::JSON_WORD_STR).empty());

        REQUIRE(m_val.find(args::COLUMN_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::DISTANCES_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::OUTPUT_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::ROW_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::SEED_WORD_STR) != m_val.cend());
    }

    SECTION("Parse JSON string 2") {
        vector<string> args_vec = { args::JSON_OPTION_STR, VALID_JSON_STR_2 };
        REQUIRE(args_handler.parse(args_vec));

        const auto& m = args_handler.get();
        REQUIRE(m.has_value());

        // Test all forms of access for JSON
        const auto& m_val = m.value();
        REQUIRE_FALSE(m_val.at(args::JSON_FLAG_STR).empty());
        REQUIRE_FALSE(m_val.at(args::JSON_OPTION_STR).empty());
        REQUIRE_FALSE(m_val.at(args::JSON_WORD_STR).empty());

        REQUIRE(m_val.at(args::COLUMN_WORD_STR) == to_string(configurator::DEFAULT_COLUMNS));
        REQUIRE(m_val.at(args::ROW_WORD_STR) == to_string(configurator::DEFAULT_ROWS));
    }

    SECTION("Cannot parse JSON string") {
        vector<string> args_vec = { args::JSON_FLAG_STR, INVALID_JSON_STR_1 };
        REQUIRE_FALSE(args_handler.parse(args_vec));

        const auto& m = args_handler.get();
        REQUIRE(m.has_value());
        REQUIRE(m.value().empty());
    }
}

TEST_CASE("Args can handle a JSON file input", "[json_file_input]") {

    static constexpr auto JSON_FILE_NAME = "array.json";

    args args_handler{};

    SECTION("JSON input file") {

        string valid_json_file_input = args::JSON_OPTION_STR;
        valid_json_file_input.append("=");
        valid_json_file_input.append(JSON_FILE_NAME);

        REQUIRE(args_handler.parse(cref(valid_json_file_input)));

        const auto& m = args_handler.get();
        REQUIRE(m.has_value());

        // Test all forms of access for JSON
        const auto& m_val = m.value();
        REQUIRE_FALSE(m_val.empty());

        REQUIRE(m_val.find(args::COLUMN_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::DISTANCES_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::OUTPUT_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::ROW_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::SEED_WORD_STR) != m_val.cend());
    }
}

TEST_CASE("Args parse with argc/argv", "[parse_argc_argv]") {

    args args_handler{};

    static constexpr auto ARGC_1 = 7;

    static const string rows_str = to_string(configurator::MAX_ROWS - 1);
    static const string cols_str = to_string(configurator::MAX_COLUMNS - 1);
    static const string algo_str = to_string_from_algo(configurator::DEFAULT_ALGO_ID);

    static char* test_argv[ARGC_1] = {
        const_cast<char*>("program"),
        const_cast<char*>("-r"), const_cast<char*>(rows_str.c_str()),
        const_cast<char*>("-c"), const_cast<char*>(cols_str.c_str()),
        const_cast<char*>("-a"), const_cast<char*>(algo_str.c_str())
    };

    std::string s(test_argv[0]);

    REQUIRE(args_handler.parse(ARGC_1, test_argv));

    const auto& m = args_handler.get();
    REQUIRE(m.has_value());

    const auto& m_val = m.value();
    REQUIRE(m_val.at(args::ROW_WORD_STR) == to_string(configurator::MAX_ROWS - 1));
    REQUIRE(m_val.at(args::COLUMN_WORD_STR) == to_string(configurator::MAX_COLUMNS - 1));
    REQUIRE(m_val.at(args::ALGO_ID_WORD_STR) == to_string_from_algo(configurator::DEFAULT_ALGO_ID));
}

TEST_CASE("Args parse with string input", "[parse_string_input]") {

    args args_handler{};

    static const auto VALID_ARGS_STR = "./app -r "
        + to_string(configurator::MAX_ROWS - 1) + " -c "
        + to_string(configurator::MAX_COLUMNS - 1) + " -a "
        + to_string_from_algo(configurator::DEFAULT_ALGO_ID);

    REQUIRE(args_handler.parse(cref(VALID_ARGS_STR)));

    const auto& m = args_handler.get();
    REQUIRE(m.has_value());

    const auto& m_val = m.value();
    REQUIRE(m_val.at(args::ROW_WORD_STR) == to_string(configurator::MAX_ROWS - 1));
    REQUIRE(m_val.at(args::COLUMN_WORD_STR) == to_string(configurator::MAX_COLUMNS - 1));
    REQUIRE(m_val.at(args::ALGO_ID_WORD_STR) == to_string_from_algo(configurator::DEFAULT_ALGO_ID));
}

// Add this test case to verify the sliced array syntax for distances flag
TEST_CASE("Args are bad and does not parse", "[args_does_not_parse]") {

    args args_handler{};

    SECTION("Wrong starting bracket") {

        static const string BAD_SLICE = args::DISTANCES_FLAG_STR
            + string(" ]")
            + to_string(configurator::DEFAULT_DISTANCES_START)
            + ":" + to_string(configurator::DEFAULT_DISTANCES_END)
            + "]";

        REQUIRE_FALSE(args_handler.parse(cref(BAD_SLICE)));
    }

    SECTION("Wrong ending bracket") {

        static const string BAD_SLICE = args::DISTANCES_FLAG_STR
            + string(" [")
            + to_string(configurator::DEFAULT_DISTANCES_START)
            + ":" + to_string(configurator::DEFAULT_DISTANCES_END)
            + "[";

        REQUIRE_FALSE(args_handler.parse(cref(BAD_SLICE)));
    }

    SECTION("Missing colon") {

        static const string BAD_SLICE = args::DISTANCES_FLAG_STR
            + string(" [")
            + to_string(configurator::DEFAULT_DISTANCES_START)
            + "" + to_string(configurator::DEFAULT_DISTANCES_END)
            + "]";

        REQUIRE_FALSE(args_handler.parse(cref(BAD_SLICE)));
    }
    
    SECTION("Distances with mixed array syntax in other arguments") {
        vector<string> args_vec = { "-r", "10", "-c", "5", "-d", "[3:7]", "-s", "42" };
        REQUIRE(args_handler.parse(args_vec));
        
        // Check all arguments are preserved
        REQUIRE(args_handler.get("-r").has_value());
        REQUIRE(args_handler.get("-r").value() == "10");
        REQUIRE(args_handler.get("-c").has_value());
        REQUIRE(args_handler.get("-c").value() == "5");
        REQUIRE(args_handler.get("-s").has_value());
        REQUIRE(args_handler.get("-s").value() == "42");
        
        // Check array syntax is handled correctly
        REQUIRE(args_handler.get("-d").has_value());
        REQUIRE(args_handler.get("-d").value() == "[3:7]");
        REQUIRE(args_handler.get("distances_start").has_value());
        REQUIRE(args_handler.get("distances_start").value() == "3");
        REQUIRE(args_handler.get("distances_end").has_value());
        REQUIRE(args_handler.get("distances_end").value() == "7");
    }
}

    
