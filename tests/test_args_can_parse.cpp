#include <algorithm>
#include <format>
#include <functional>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_view_utils.h>

using namespace mazes;
using namespace std;

static constexpr auto ARRAY_DOT_JSON_FILE{ "array.json" };

static constexpr auto MAZE_DOT_JSON_FILE{ "maze.json" };

static constexpr auto OUTPUT_FILE_NAME{ "out.txt" };

TEST_CASE("Args static checks", "[args_static_checks]") {

    STATIC_REQUIRE(std::is_default_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::args>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::args>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::args>::value);

    // Used internally within the args class
    STATIC_REQUIRE(std::is_default_constructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::json_helper>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::json_helper>::value);
}

TEST_CASE("Args simple parses", "[simple_parses]") {

    args args_handler{};

    SECTION("Empty vector") {

        vector<string> args_vec;
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("App name only") {

        vector<string> args_vec = { "maze_builder" };
        REQUIRE(args_handler.parse(args_vec, true));
    }

    SECTION("Help requested with vector of string") {

        vector<string> args_vec = { args::HELP_FLAG_STR };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Version requested with vector of string") {

        vector<string> args_vec = { args::VERSION_FLAG_STR };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Help and version requested short flags") {

        vector<string> args_vec = { args::VERSION_FLAG_STR, args::HELP_FLAG_STR };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Help and version requested options") {

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
        
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_FLAG_STR), to_string_from_algo(ALGO)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_OPTION_STR), to_string_from_algo(ALGO)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), to_string_from_algo(ALGO)));
    }

    SECTION("Parse and get output value") {
        vector<string> args_vec = { args::OUTPUT_ID_FLAG_STR, OUTPUT_FILE_NAME };
        REQUIRE(args_handler.parse(args_vec));

        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_FLAG_STR), OUTPUT_FILE_NAME));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_OPTION_STR), OUTPUT_FILE_NAME));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), OUTPUT_FILE_NAME));
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
        auto slice_1_stripped = string_view_utils::strip(DISTANCES_SLICE_1, args::DISTANCES_FLAG_STR);
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), slice_1_stripped));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), slice_1_stripped));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), slice_1_stripped));
        
        // Test parsed slice values
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_START_STR), to_string(DISTANCES_START)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_END_STR), to_string(DISTANCES_END)));

        args_handler.clear();

        string rev_args = { DISTANCES_SLICE_1.crbegin(), DISTANCES_SLICE_1.crend() };
        REQUIRE_FALSE(args_handler.parse(cref(rev_args)));

        args_handler.clear();

        REQUIRE(args_handler.parse(cref(DISTANCES_SLICE_2)));

        // Test all forms of access for distances with slice
        auto slice_2_stripped = string_view_utils::strip(string_view_utils::strip(DISTANCES_SLICE_2, args::DISTANCES_OPTION_STR), "=");
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), slice_2_stripped));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), slice_2_stripped));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), slice_2_stripped));

        // Test parsed slice values
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_START_STR), to_string(DISTANCES_START)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_END_STR), to_string(DISTANCES_END)));

        args_handler.clear();

        REQUIRE(args_handler.parse(cref(DISTANCES_SLICE_3)));

        // Test all forms of access for distances with slice
        auto slice_3_stripped = string_view_utils::strip(DISTANCES_SLICE_3, args::DISTANCES_FLAG_STR);
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_FLAG_STR), slice_3_stripped));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_OPTION_STR), slice_3_stripped));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), slice_3_stripped));

        // Test parsed slice values
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_START_STR), to_string(DISTANCES_START)));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_END_STR), to_string(DISTANCES_END)));
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
            "output": "invalidjsonstr.txt"
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
        "  `{\n\"rows\": " + to_string(configurator::DEFAULT_ROWS)
        + ",\n\"columns\": " + to_string(configurator::DEFAULT_COLUMNS)
        + "\n }`";

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
        vector<string> args_vec = { args::JSON_OPTION_STR + string("=") + VALID_JSON_STR_2 };
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
        REQUIRE_FALSE(m.value().empty());
    }
}

TEST_CASE("Args can handle a JSON file input", "[json_file_input]") {

    args args_handler{};

    SECTION("JSON input file") {

        string valid_json_file_input = format("{}={}", args::JSON_OPTION_STR, ARRAY_DOT_JSON_FILE);

        REQUIRE(args_handler.parse(cref(valid_json_file_input), false));

        const auto& m = args_handler.get();
        REQUIRE(m.has_value());

        // Test fields are indexed (from first object in array for backward compatibility)
        const auto& m_val = m.value();
        REQUIRE_FALSE(m_val.empty());

        REQUIRE(m_val.find(args::COLUMN_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::DISTANCES_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::OUTPUT_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::ROW_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::SEED_WORD_STR) != m_val.cend());
    }
}

TEST_CASE("Args can handle JSON array files", "[json_array_input]") {

    args args_handler{};

    SECTION("JSON array file input") {

        string valid_json_file_input = args::JSON_OPTION_STR;
        valid_json_file_input.append("=");
        valid_json_file_input.append(ARRAY_DOT_JSON_FILE);

        REQUIRE(args_handler.parse(cref(valid_json_file_input)));

        // Test the new array functionality
        const auto& array_opt = args_handler.get_array();
        REQUIRE(array_opt.has_value());
        
        const auto& array_val = array_opt.value();
        REQUIRE(array_val.size() == 4); // We expect 4 objects in array.json
        
        // Test first object in array
        const auto& first_config = array_val[0];
        REQUIRE(first_config.find("rows") != first_config.cend());
        REQUIRE(first_config.find("columns") != first_config.cend());
        REQUIRE(first_config.find("levels") != first_config.cend());
        REQUIRE(first_config.find("seed") != first_config.cend());
        REQUIRE(first_config.find("algo") != first_config.cend());
        REQUIRE(first_config.find("output") != first_config.cend());
        REQUIRE(first_config.find("distances") != first_config.cend());
        
        // Verify values from array.json
        REQUIRE(first_config.at("rows") == "10");
        REQUIRE(first_config.at("columns") == "20");
        REQUIRE(first_config.at("levels") == "30");
        REQUIRE(first_config.at("seed") == "9000000");
        REQUIRE(first_config.at("algo") == "\"dfs\"");
        REQUIRE(first_config.at("output") == "\"maze_dfs.txt\"");
        REQUIRE(first_config.at("distances") == "true");
        
        // Test second object in array
        const auto& second_config = array_val[1];
        REQUIRE(second_config.at("rows") == "20");
        REQUIRE(second_config.at("columns") == "20");
        REQUIRE(second_config.at("levels") == "3");
        REQUIRE(second_config.at("seed") == "9");
        REQUIRE(second_config.at("algo") == "\"dfs\"");
        REQUIRE(second_config.at("output") == "\"maze_dfs2.txt\"");
        REQUIRE(second_config.at("distances") == "false");
        
        // Test last object in array
        const auto& last_config = array_val[3];
        REQUIRE(last_config.at("rows") == "50");
        REQUIRE(last_config.at("columns") == "50");
        REQUIRE(last_config.at("levels") == "50");
        REQUIRE(last_config.at("seed") == "10");
        REQUIRE(last_config.at("algo") == "\"binary_tree\"");
        REQUIRE(last_config.at("output") == "\"maze_bt.txt\"");
        REQUIRE(last_config.at("distances") == "false");
        
        // Test backward compatibility - single map should contain first object
        const auto& m = args_handler.get();
        REQUIRE(m.has_value());
        const auto& m_val = m.value();
        REQUIRE_FALSE(m_val.empty());
        
        // Should have all the argument variations from first object
        REQUIRE(m_val.find(args::ROW_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::COLUMN_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::LEVEL_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::SEED_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::ALGO_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::OUTPUT_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::DISTANCES_WORD_STR) != m_val.cend());
    }
}

TEST_CASE("Args parse with argc/argv", "[parse_argc_argv]") {

    args args_handler{};

    static constexpr auto ARGC_7 = 7;

    static const string rows_str = to_string(configurator::MAX_ROWS - 1);
    static const string cols_str = to_string(configurator::MAX_COLUMNS - 1);
    static const string algo_str = to_string_from_algo(configurator::DEFAULT_ALGO_ID);

    static char* test_argv[ARGC_7] = {
        const_cast<char*>("program"),
        const_cast<char*>("-r"), const_cast<char*>(rows_str.c_str()),
        const_cast<char*>("-c"), const_cast<char*>(cols_str.c_str()),
        const_cast<char*>("-a"), const_cast<char*>(algo_str.c_str())
    };

    std::string s(test_argv[0]);

    REQUIRE(args_handler.parse(ARGC_7, test_argv, true));

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

    REQUIRE(args_handler.parse(cref(VALID_ARGS_STR), true));

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
    
    SECTION("Brackets without colon") {
        vector<string> args_vec = { "app", "-d", "[123]" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Malformed slice with reversed brackets") {
        vector<string> args_vec = { "app", "-d", "]1:2[" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Only closing bracket") {
        vector<string> args_vec = { "app", "-d", "1:2]" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Only opening bracket") {
        vector<string> args_vec = { "app", "-d", "[1:2" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Mixed valid and invalid arguments") {
        vector<string> args_vec = { "app", "-r", "10", "-d", "]1:2]", "-c", "5" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Distances with mixed array syntax in other arguments") {
        vector<string> args_vec = { "-r", "10", "-c", "5", "--distances=[3:7]", "-s", "42" };
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

TEST_CASE("Args enhanced valid parsing", "[enhanced_valid_parsing]") {
    
    args args_handler{};
    
    auto check_optional_equals_value = [](auto opt, auto val) -> bool {
        return opt.has_value() && opt.value() == val;
    };

    SECTION("App name only") {
        vector<string> args_vec = { "app" };
        REQUIRE(args_handler.parse(args_vec, true));
    }

    SECTION("App with seed only") {
        vector<string> args_vec = { "app", "-s", "2" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_FLAG_STR), "2"));
    }

    SECTION("App with algorithm dfs") {
        vector<string> args_vec = { "app", "-a", "dfs" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_FLAG_STR), "dfs"));
    }

    SECTION("App with algorithm binary_tree") {
        vector<string> args_vec = { "app", "-a", "binary_tree" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_FLAG_STR), "binary_tree"));
    }

    SECTION("App with algorithm sidewinder") {
        vector<string> args_vec = { "app", "-asidewinder" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_FLAG_STR), "sidewinder"));
    }

    SECTION("App with long options using equals") {
        vector<string> args_vec = { "app", "--rows=10", "--columns=10", "--seed=2" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
    }

    SECTION("App with levels option") {
        vector<string> args_vec = { "app", "--rows=1", "--columns=2", "--levels=3" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "1"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "2"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::LEVEL_WORD_STR), "3"));
    }

    SECTION("Complex argument mix with spaces") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2", "-a", "dfs", "-o", "stdout", "-d" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), "dfs"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "stdout"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("Long format with mixed options") {
        vector<string> args_vec = { "app", "--rows=10", "--columns=10", "--seed=2", "--algo=binary_tree", "--output=1.txt", "--distances" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), "binary_tree"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "1.txt"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("Mixed short and long with obj output") {
        vector<string> args_vec = { "app", "--rows=10", "--columns=10", "--seed=2", "-a", "dfs", "--output=1.obj", "-d" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), "dfs"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "1.obj"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("With help and other options") {
        vector<string> args_vec = { "app", "--rows=10", "--columns=10", "--seed=2", "--algo=binary_tree", "--output=1.png", "-h" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), "binary_tree"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "1.png"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::HELP_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("With version and other options") {
        vector<string> args_vec = { "app", "--rows=10", "--columns=10", "--seed=2", "--algo=binary_tree", "--output=1.jpg", "-v" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), "binary_tree"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "1.jpg"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::VERSION_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("Simple short args") {
        vector<string> args_vec = { "app", "-r", "10", "-c", "10", "-s", "2" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::SEED_WORD_STR), "2"));
    }

    SECTION("With levels short option") {
        vector<string> args_vec = { "app", "-l", "5", "-r", "10", "-c", "10" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::LEVEL_WORD_STR), "5"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
    }

    SECTION("Mixed long and short") {
        vector<string> args_vec = { "app", "--rows=10", "-c", "10" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ROW_WORD_STR), "10"));
        REQUIRE(check_optional_equals_value(args_handler.get(args::COLUMN_WORD_STR), "10"));
    }

    SECTION("Single algorithm option") {
        vector<string> args_vec = { "app", "--algo=dfs" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::ALGO_ID_WORD_STR), "dfs"));
    }

    SECTION("Output to stdout") {
        vector<string> args_vec = { "app", "--output=stdout" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "stdout"));
    }

    SECTION("Output short with json") {
        vector<string> args_vec = { "app", "-o", "1.json" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "1.json"));
    }

    SECTION("Output long with json") {
        vector<string> args_vec = { "app", "--output=json" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::OUTPUT_ID_WORD_STR), "json"));
    }

    SECTION("Distances flag only") {
        vector<string> args_vec = { "app", "-d" };
        REQUIRE(args_handler.parse(args_vec, true));
        REQUIRE(check_optional_equals_value(args_handler.get(args::DISTANCES_WORD_STR), args::TRUE_VALUE));
    }

    SECTION("Fails to find app name") {
        vector<string> args_vec = { "--json=2.json", "app"};
        REQUIRE_FALSE(args_handler.parse(args_vec, true));
    }
}

TEST_CASE("Args enhanced invalid parsing", "[enhanced_invalid_parsing]") {
    
    args args_handler{};

    SECTION("Invalid long option missing dashes") {
        vector<string> args_vec = { "app", "-output" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid long option missing dashes for algorithm") {
        vector<string> args_vec = { "app", "-algorithm" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid single dash") {
        vector<string> args_vec = { "app", "-" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid arguments with bad values") {
        vector<string> args_vec = { "app", "-r", "x", "-c", "z", "-s", "algo" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid unknown short option") {
        vector<string> args_vec = { "app", "-z" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid numeric short option") {
        vector<string> args_vec = { "app", "-1" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid positional argument") {
        vector<string> args_vec = { "app", "10" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid single character positional") {
        vector<string> args_vec = { "app", "b" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid unknown long option") {
        vector<string> args_vec = { "app", "--file" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid unknown short option f") {
        vector<string> args_vec = { "app", "-f" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid option values with letters") {
        vector<string> args_vec = { "app", "--rows=r", "--columns=c", "--levels=l" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid malformed equals syntax") {
        vector<string> args_vec = { "app", "--rows=--columns=" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid spaced equals syntax") {
        vector<string> args_vec = { "app", "--columns", "=", "--rows=" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid typo in option name") {
        vector<string> args_vec = { "app", "--roows=", "--columns=" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid distances option name") {
        vector<string> args_vec = { "app", "--rows=10", "--columns=10", "--seed=2", "--algo=binary_tree", "--output=1.txt", "--distancesz" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Invalid concatenated options") {
        vector<string> args_vec = { "app", "-rx", "-cz", "-salgo" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }

    SECTION("Conflicting repeated arguments") {
        vector<string> args_vec = { "app", "-r", "2", "--rows", "2", "-r", "3" };
        // CLI11 typically allows this and uses the last value, so this might actually parse successfully
        // The behavior depends on CLI11 configuration, but let's test what happens
        bool parse_result = args_handler.parse(args_vec);
        if (parse_result) {
            // If it parses, the last value should win
            auto result = args_handler.get(args::ROW_WORD_STR);
            REQUIRE(result.has_value());
            REQUIRE(result.value() == "3");
        } else {
            // If it doesn't parse, that's also acceptable behavior
            REQUIRE_FALSE(parse_result);
        }
    }
}

TEST_CASE("Args validation with distances slices", "[args_validation_with_slices]") {
    
    args args_handler{};
    
    SECTION("Valid slice syntax should pass") {
        vector<string> args_vec = { "app", "-d", "[1:5]" };
        REQUIRE(args_handler.parse(args_vec, true));
    }
    
    SECTION("Valid option=value slice syntax should pass") {
        vector<string> args_vec = { "app", "--distances=[1:5]" };
        REQUIRE(args_handler.parse(args_vec, true));
    }
    
    SECTION("Malformed slice - wrong starting bracket should fail") {
        vector<string> args_vec = { "app", "-d", "]1:5]" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Malformed slice - wrong ending bracket should fail") {
        vector<string> args_vec = { "app", "-d", "[1:5[" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Malformed slice - missing colon should fail") {
        vector<string> args_vec = { "app", "-d", "[15]" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Malformed slice - only opening bracket should fail") {
        vector<string> args_vec = { "app", "-d", "[1:5" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Malformed slice - only closing bracket should fail") {
        vector<string> args_vec = { "app", "-d", "1:5]" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Malformed slice - colon without brackets should fail") {
        vector<string> args_vec = { "app", "-d", "1:5" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Valid flag without value should pass") {
        vector<string> args_vec = { "app", "-d" };
        REQUIRE(args_handler.parse(args_vec, true));
    }
    
    SECTION("Unknown option should fail") {
        vector<string> args_vec = { "app", "-z" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Unexpected positional argument should fail") {
        vector<string> args_vec = { "app", "unexpected" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
    
    SECTION("Valid concatenated short option should pass") {
        vector<string> args_vec = { "app", "-r10" };
        REQUIRE(args_handler.parse(args_vec, true));
    }
    
    SECTION("Invalid concatenated option should fail") {
        vector<string> args_vec = { "app", "-z10" };
        REQUIRE_FALSE(args_handler.parse(args_vec));
    }
}

TEST_CASE("Args backward compatibility with single JSON objects", "[json_single_object]") {

    args args_handler{};

    SECTION("JSON single object file input") {

        string valid_json_file_input = args::JSON_OPTION_STR;
        valid_json_file_input.append("=");
        valid_json_file_input.append(MAZE_DOT_JSON_FILE);

        REQUIRE(args_handler.parse(cref(valid_json_file_input)));

        // Test single map functionality (backward compatibility)
        const auto& m = args_handler.get();
        REQUIRE(m.has_value());
        const auto& m_val = m.value();
        REQUIRE_FALSE(m_val.empty());
        
        // Should have all the argument variations
        REQUIRE(m_val.find(args::ROW_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::COLUMN_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::LEVEL_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::SEED_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::ALGO_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::OUTPUT_ID_WORD_STR) != m_val.cend());
        REQUIRE(m_val.find(args::DISTANCES_WORD_STR) != m_val.cend());
        
        // Verify expected values
        REQUIRE(m_val.at(args::ROW_WORD_STR) == "10");
        REQUIRE(m_val.at(args::COLUMN_WORD_STR) == "20");
        REQUIRE(m_val.at(args::LEVEL_WORD_STR) == "30");
        REQUIRE(m_val.at(args::SEED_WORD_STR) == "9001");
        REQUIRE(m_val.at(args::ALGO_ID_WORD_STR) == "\"dfs\"");
        REQUIRE(m_val.at(args::OUTPUT_ID_WORD_STR) == "\"maze_dfs.txt\"");
        REQUIRE(m_val.at(args::DISTANCES_WORD_STR) == args::TRUE_VALUE);

        // Test array functionality should have 1 object in array
        const auto& array_opt = args_handler.get_array();
        REQUIRE(array_opt.has_value());
        const auto& array_val = array_opt.value();
        REQUIRE(array_val.size() == 1);
    }
}
