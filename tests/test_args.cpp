#include <string>
#include <functional>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>

using namespace std;
using namespace mazes;

TEST_CASE("Args parses correctly", "[good parses]") {
    args args_handler{};

    SECTION("Help requested") {
        vector<string> args_vec = { "-h", "--help" };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Version requested") {
        vector<string> args_vec = { "--version", "-v" };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Help and version requested") {
        vector<string> args_vec = { "-h", "-v" };
        REQUIRE(args_handler.parse(args_vec));
    }

    SECTION("Short arguments 1") {
        vector<string> args_vec = { "-s", "500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto val = args_handler.get("-s");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "500");
    }

    SECTION("Short arguments 2") {
        vector<string> args_vec = { "-r", "10", "-s", "500" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        auto val_s = args_handler.get("-s");
        REQUIRE(val_s.has_value());
        REQUIRE(val_s.value() == "500");
        
        auto val_r = args_handler.get("-r");
        REQUIRE(val_r.has_value());
        REQUIRE(val_r.value() == "10");
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
        REQUIRE(args_handler.get("--output").has_value());
        REQUIRE(args_handler.get("--algo").has_value());
        auto algo_val = args_handler.get("--algo");
        REQUIRE(algo_val == "binary_tree");
    }

    SECTION("No args") {
        string none = "";
        REQUIRE(args_handler.parse(cref(none)));
    }
}

TEST_CASE("Args add options and flags", "[options and flags]") {
    args args_handler{};
    
    SECTION("Add option") {
        REQUIRE(args_handler.add_option("-r,--rows", "Number of rows in the maze"));
        vector<string> args_vec = { "--rows", "15" };
        REQUIRE(args_handler.parse(args_vec));
        auto val = args_handler.get("--rows");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "15");
    }
    
    SECTION("Add flag") {
        REQUIRE(args_handler.add_flag("-d,--distances", "Calculate distances"));
        vector<string> args_vec = { "--distances" };
        REQUIRE(args_handler.parse(args_vec));
        auto val = args_handler.get("--distances");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "true");
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
}

TEST_CASE("Args can handle a JSON input file", "[json input file]") {
    args args_handler{};
    SECTION("JSON input file") {
        string json_file_valid = " -j mazes_array.json  ";
        REQUIRE(args_handler.parse(cref(json_file_valid)));

        const auto& m = args_handler.get();
        REQUIRE(m.find("rows") != m.end());
        REQUIRE(m.find("columns") != m.end());
        REQUIRE(m.find("seed") != m.end());
        REQUIRE(m.find("distances") != m.end());
        REQUIRE(m.find("output") != m.end());
    }
}

TEST_CASE("Args can handle a JSON arr of objects", "[json arr input]") {
    args args_handler{};
    SECTION("JSON arr input string") {
        string json_array = R"json(-j `[
            {
                "rows": 10,
                "columns": 20,
                "levels": 30,
                "seed": 9000000,
                "algo": "dfs",
                "output": "maze_dfs.txt",
                "distances": true
            },
            {
                "rows": 20,
                "columns": 20,
                "levels": 3,
                "seed": 9,
                "algo": "dfs",
                "output": "maze_dfs2.txt",
                "distances": false
            }
        ]`)json";
        
        REQUIRE(args_handler.parse(cref(json_array)));
        REQUIRE(args_handler.has_array());
        
        // Check that the arr was properly parsed
        const auto& arr = args_handler.get_array();
        REQUIRE(arr.size() == 2);
        
        // Check first config
        REQUIRE(arr[0].find("rows") != arr[0].end());
        REQUIRE(arr[0].find("columns") != arr[0].end());
        REQUIRE(arr[0].find("levels") != arr[0].end());
        REQUIRE(arr[0].find("seed") != arr[0].end());
        REQUIRE(arr[0].find("algo") != arr[0].end());
        REQUIRE(arr[0].find("output") != arr[0].end());
        REQUIRE(arr[0].find("distances") != arr[0].end());
    }
    
    SECTION("JSON arr input file") {
        string json_file = " -j mazes_array.json  ";
        REQUIRE(args_handler.parse(cref(json_file)));
        REQUIRE(args_handler.has_array());
        
        // Check that the arr was properly loaded from file
        const auto& arr = args_handler.get_array();
        REQUIRE(arr.size() == 4);
    }
    
    SECTION("to_str() serializes JSON arrays correctly") {
        string json_file = " -j mazes_array.json  ";
        REQUIRE(args_handler.parse(cref(json_file)));
        REQUIRE(args_handler.has_array());
        
        std::string str_output = args::to_str(args_handler);
        REQUIRE(!str_output.empty());
    }
}

// Add this test case at the end of the file
TEST_CASE("Args correctly handles output file specification with JSON array input", "[json output]") {
    args args_handler{};

    SECTION("Output file with JSON array input") {
        vector<string> args_vec = { "-o", "out.json", "-j", "mazes_array.json" };
        REQUIRE(args_handler.parse(cref(args_vec)));

        // Check that both -o and -j arguments are preserved
        REQUIRE(args_handler.get("-o").has_value());
        REQUIRE(args_handler.get("-o").value() == "out.json");
        REQUIRE(args_handler.get("-j").has_value());
        REQUIRE(args_handler.get("-j").value() == "mazes_array.json");
    }

    SECTION("Output file with JSON array input using long options") {
        vector<string> args_vec = { "--output=out.json", "--json=mazes_array.json" };
        REQUIRE(args_handler.parse(cref(args_vec)));

        // Check that both output and json arguments are preserved
        REQUIRE(args_handler.get("--output").has_value());
        REQUIRE(args_handler.get("--output").value() == "out.json");
        REQUIRE(args_handler.get("--json").has_value());
        REQUIRE(args_handler.get("--json").value() == "mazes_array.json");
    }
}

// Add this test case at the end of the file
TEST_CASE("Args correctly handles automatic JSON output file generation", "[json auto output]") {
    args args_handler{};

    SECTION("JSON input file with automatic output naming") {
        // Case where -j input.json is provided without explicit output
        vector<string> args_vec = { "-j", "input.json" };
        REQUIRE(args_handler.parse(cref(args_vec)));

        // Check that -j argument is preserved
        REQUIRE(args_handler.get("-j").has_value());
        REQUIRE(args_handler.get("-j").value() == "input.json");
        
        // Check that -o was automatically set to output.json
        REQUIRE(args_handler.get("-o").has_value());
        REQUIRE(args_handler.get("-o").value() == "input_out.json");
    }

    SECTION("JSON input file with user-specified output") {
        // Case where both -j input.json and -o custom.json are provided
        vector<string> args_vec = { "-j", "input.json", "-o", "custom.json" };
        REQUIRE(args_handler.parse(cref(args_vec)));

        // Check that both -j and -o arguments are preserved
        REQUIRE(args_handler.get("-j").has_value());
        REQUIRE(args_handler.get("-j").value() == "input.json");
        REQUIRE(args_handler.get("-o").has_value());
        REQUIRE(args_handler.get("-o").value() == "custom.json");
    }
}

TEST_CASE("Args parse with argc/argv", "[parse argc argv]") {
    args args_handler{};
    
    SECTION("Basic argc/argv parsing") {
        char* test_argv[] = {
            (char*)"program",
            (char*)"-r", (char*)"10",
            (char*)"-c", (char*)"15",
            (char*)"-j", (char*)"input.json",
            nullptr
        };
        int test_argc = 7;
        
        REQUIRE(args_handler.parse(test_argc, test_argv));
        
        // Verify args were parsed
        REQUIRE(args_handler.get("-j").has_value());
        REQUIRE(args_handler.get("-j").value() == "input.json");
    }
}

// Add this test case to specifically test the short-form argument parsing issue
TEST_CASE("Args correctly parses and accesses short-form arguments", "[short form args]") {
    args args_handler{};

    SECTION("Short form with spaces and accessor testing") {
        // Test the specific case "-r 10 -c 5"
        vector<string> args_vec = { "-r", "10", "-c", "5" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        
        // Verify we can access values using all forms of keys
        // First check short form with dash
        REQUIRE(args_handler.get("-r").has_value());
        REQUIRE(args_handler.get("-r").value() == "10");
        REQUIRE(args_handler.get("-c").has_value());
        REQUIRE(args_handler.get("-c").value() == "5");
        
        // Check long form with dashes
        REQUIRE(args_handler.get("--rows").has_value());
        REQUIRE(args_handler.get("--rows").value() == "10");
        REQUIRE(args_handler.get("--columns").has_value());
        REQUIRE(args_handler.get("--columns").value() == "5");
        
        // Check form without dashes
        REQUIRE(args_handler.get("rows").has_value());
        REQUIRE(args_handler.get("rows").value() == "10");
        REQUIRE(args_handler.get("columns").has_value());
        REQUIRE(args_handler.get("columns").value() == "5");
    }

    SECTION("Short form as string") {
        // Test the string version of command line
        string args_str = "-r 10 -c 5";
        REQUIRE(args_handler.parse(cref(args_str)));
        
        // Verify using short form
        REQUIRE(args_handler.get("-r").has_value());
        REQUIRE(args_handler.get("-r").value() == "10");
        REQUIRE(args_handler.get("-c").has_value());
        REQUIRE(args_handler.get("-c").value() == "5");
        
        // Also verify long form equivalents
        REQUIRE(args_handler.get("--rows").has_value());
        REQUIRE(args_handler.get("--rows").value() == "10");
        REQUIRE(args_handler.get("--columns").has_value());
        REQUIRE(args_handler.get("--columns").value() == "5");
    }

    SECTION("Short form with argc/argv") {
        // Test with argc/argv which is how it would be called in a real program
        char* test_argv[] = {
            (char*)"program",
            (char*)"-r", (char*)"10",
            (char*)"-c", (char*)"5",
            nullptr
        };
        int test_argc = 5;
        
        REQUIRE(args_handler.parse(test_argc, test_argv));
        
        // Check all forms of access
        REQUIRE(args_handler.get("-r").has_value());
        REQUIRE(args_handler.get("-r").value() == "10");
        REQUIRE(args_handler.get("-c").has_value());
        REQUIRE(args_handler.get("-c").value() == "5");
        
        // Check that get() returns a map containing all the values
        const auto& map = args_handler.get();
        REQUIRE(map.find("-r") != map.end());
        REQUIRE(map.at("-r") == "10");
        REQUIRE(map.find("-c") != map.end());
        REQUIRE(map.at("-c") == "5");
        
        // Check long form access
        REQUIRE(args_handler.get("--rows").has_value());
        REQUIRE(args_handler.get("--rows").value() == "10");
    }
    
    SECTION("Mixed short/long form arguments") {
        // Test with mixed short and long form
        vector<string> args_vec = { "-r", "10", "--columns", "5" };
        REQUIRE(args_handler.parse(cref(args_vec)));
        
        // Should be able to access with any form
        REQUIRE(args_handler.get("-r").has_value());
        REQUIRE(args_handler.get("-r").value() == "10");
        REQUIRE(args_handler.get("--columns").has_value());
        REQUIRE(args_handler.get("--columns").value() == "5");
        
        // Check cross-access (short/long)
        REQUIRE(args_handler.get("--rows").has_value());
        REQUIRE(args_handler.get("--rows").value() == "10");
        REQUIRE(args_handler.get("-c").has_value());
        REQUIRE(args_handler.get("-c").value() == "5");
    }
}
