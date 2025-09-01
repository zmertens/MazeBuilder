#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cctype>
#include <deque>
#include <list>
#include <memory>
#include <random>
#include <type_traits>

#include <MazeBuilder/string_utils.h>

TEST_CASE("string_utils template split functions", "[string_utils template_split]") {

    using namespace mazes;
    using namespace std;

    SECTION("Template split with vector of chars and char separator") {

        string test_string = "hello,world,test";
        vector<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, ',');
        
        REQUIRE(result.size() == 3);
        
        vector<char> expected1 = {'h', 'e', 'l', 'l', 'o'};
        vector<char> expected2 = {'w', 'o', 'r', 'l', 'd'};
        vector<char> expected3 = {'t', 'e', 's', 't'};
        
        REQUIRE(result[0] == expected1);
        REQUIRE(result[1] == expected2);
        REQUIRE(result[2] == expected3);
    }
    
    SECTION("Template split with custom predicate") {

        string test_string = "1a2a3a4";
        vector<vector<char>> result;
        
        auto custom_pred = [](const char& el, const char& sep) {
            return el == sep;
        };
        
        string_utils::split(test_string.begin(), test_string.end(), result, 'a', custom_pred);
        
        REQUIRE(result.size() == 4);
        REQUIRE(result[0] == vector<char>{'1'});
        REQUIRE(result[1] == vector<char>{'2'});
        REQUIRE(result[2] == vector<char>{'3'});
        REQUIRE(result[3] == vector<char>{'4'});
    }
    
    SECTION("strsplit with string and vector<string>") {

        string input = "apple|banana|cherry";
        vector<string> result;
        
        string_utils::strsplit(input, result, '|');
        
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "apple");
        REQUIRE(result[1] == "banana");
        REQUIRE(result[2] == "cherry");
    }
    
    SECTION("strsplit with vector of integers") {

        vector<int> input = {1, 9, 2, 9, 3, 9, 4};
        vector<vector<int>> result;
        
        string_utils::strsplit(input, result, 9);
        
        REQUIRE(result.size() == 4);
        REQUIRE(result[0] == vector<int>{1});
        REQUIRE(result[1] == vector<int>{2});
        REQUIRE(result[2] == vector<int>{3});
        REQUIRE(result[3] == vector<int>{4});
    }
    
    SECTION("Template split with empty input") {

        string empty_string = "";
        vector<vector<char>> result;
        
        auto end_it = string_utils::split(empty_string.begin(), empty_string.end(), result, ',');
        
        REQUIRE(result.empty());
        REQUIRE(end_it == empty_string.end());
    }
    
    SECTION("Template split with no separators") {

        string test_string = "noseparators";
        vector<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, ',');
        
        REQUIRE(result.size() == 1);
        vector<char> expected(test_string.begin(), test_string.end());
        REQUIRE(result[0] == expected);
    }
    
    SECTION("Template split with consecutive separators") {

        string test_string = "a,,b,,c";
        vector<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, ',');
        
        REQUIRE(result.size() == 5);
        REQUIRE(result[0] == vector<char>{'a'});
        REQUIRE(result[1].empty());  // Empty between consecutive commas
        REQUIRE(result[2] == vector<char>{'b'});
        REQUIRE(result[3].empty());  // Empty between consecutive commas
        REQUIRE(result[4] == vector<char>{'c'});
    }
    
    SECTION("Template split with list output container") {

        string test_string = "x-y-z";
        list<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, '-');
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == vector<char>{'x'});
        ++it;
        REQUIRE(*it == vector<char>{'y'});
        ++it;
        REQUIRE(*it == vector<char>{'z'});
    }
    
    SECTION("strsplit with string_view input") {

        string_view input = "data:info:value";
        vector<string> result;
        
        string_utils::strsplit(input, result, ':');
        
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "data");
        REQUIRE(result[1] == "info");
        REQUIRE(result[2] == "value");
    }
    
    SECTION("Template split with numeric predicate") {

        vector<int> numbers = {10, 5, 20, 5, 30, 5, 40};
        vector<vector<int>> result;
        
        auto numeric_pred = [](const int& el, const int& sep) {
            return el == sep;
        };
        
        string_utils::split(numbers.begin(), numbers.end(), result, 5, numeric_pred);
        
        REQUIRE(result.size() == 4);
        REQUIRE(result[0] == vector<int>{10});
        REQUIRE(result[1] == vector<int>{20});
        REQUIRE(result[2] == vector<int>{30});
        REQUIRE(result[3] == vector<int>{40});
    }
}

TEST_CASE("string_utils template split edge cases and compatibility", "[string_utils template_edge_cases]") {

    using namespace mazes;
    using namespace std;

    SECTION("Template split with single character input") {

        string single_char = "a";
        vector<vector<char>> result;
        
        string_utils::split(single_char.begin(), single_char.end(), result, ',');
        
        REQUIRE(result.size() == 1);
        REQUIRE(result[0] == vector<char>{'a'});
    }
    
    SECTION("Template split starting with separator") {

        string test_string = ",hello,world";
        vector<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, ',');
        
        REQUIRE(result.size() == 3);
        REQUIRE(result[0].empty());  // Empty first part
        REQUIRE(result[1] == vector<char>{'h', 'e', 'l', 'l', 'o'});
        REQUIRE(result[2] == vector<char>{'w', 'o', 'r', 'l', 'd'});
    }
    
    SECTION("Template split ending with separator") {

        string test_string = "hello,world,";
        vector<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, ',');
        
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == vector<char>{'h', 'e', 'l', 'l', 'o'});
        REQUIRE(result[1] == vector<char>{'w', 'o', 'r', 'l', 'd'});
    }
    
    SECTION("Template split with deque output container") {

        string test_string = "a;b;c;d";
        deque<vector<char>> result;
        
        string_utils::split(test_string.begin(), test_string.end(), result, ';');
        
        REQUIRE(result.size() == 4);
        REQUIRE(result[0] == vector<char>{'a'});
        REQUIRE(result[1] == vector<char>{'b'});
        REQUIRE(result[2] == vector<char>{'c'});
        REQUIRE(result[3] == vector<char>{'d'});
    }
    
    SECTION("strsplit compatibility with existing split functionality") {

        // Test that template functions can complement existing functions
        string test_input = "alpha,beta,gamma";
        
        // Using existing split function
        list<string> results;
        string_utils::split(test_input.cbegin(), test_input.cend(), results, ',');
        
        // Using new template strsplit function
        vector<string> template_result;
        string_utils::strsplit(test_input, template_result, ',');
        
        // Compare results - convert list to vector for comparison
        vector<string> existing_as_vector(results.cbegin(), results.cend());

        REQUIRE(existing_as_vector.size() == template_result.size());
        REQUIRE(existing_as_vector == template_result);
    }
    
    SECTION("Template split with complex custom predicate") {

        string test_string = "1a2A3a4A5";
        vector<vector<char>> result;
        
        // Custom predicate that matches both 'a' and 'A'
        auto case_insensitive_pred = [](const char& el, const char& sep) {
            return tolower(el) == tolower(sep);
        };
        
        string_utils::split(test_string.begin(), test_string.end(), result, 'a', case_insensitive_pred);
        
        REQUIRE(result.size() == 5);
        REQUIRE(result[0] == vector<char>{'1'});
        REQUIRE(result[1] == vector<char>{'2'});
        REQUIRE(result[2] == vector<char>{'3'});
        REQUIRE(result[3] == vector<char>{'4'});
        REQUIRE(result[4] == vector<char>{'5'});
    }
    
    SECTION("Template split return iterator value") {

        string test_string = "one,two,three";
        vector<vector<char>> result;
        
        auto end_it = string_utils::split(test_string.begin(), test_string.end(), result, ',');
        
        REQUIRE(end_it == test_string.end());
        REQUIRE(result.size() == 3);
    }
    
    SECTION("strsplit with string containing maze-like characters") {

        // Test compatibility with maze building context
        string maze_chars = "+|-+|-+";
        vector<string> result;
        
        string_utils::strsplit(maze_chars, result, '|');
        
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "+");
        REQUIRE(result[1] == "-+");
        REQUIRE(result[2] == "-+");
    }
}

TEST_CASE("string_utils template split practical examples", "[string_utils practical]") {

    using namespace mazes;
    using namespace std;

    SECTION("Parse maze coordinates using template split") {

        // Example: parsing coordinate pairs like "1,2;3,4;5,6"
        string coord_string = "1,2;3,4;5,6";
        vector<string> coord_pairs;
        
        string_utils::strsplit(coord_string, coord_pairs, ';');
        
        REQUIRE(coord_pairs.size() == 3);
        REQUIRE(coord_pairs[0] == "1,2");
        REQUIRE(coord_pairs[1] == "3,4");
        REQUIRE(coord_pairs[2] == "5,6");
        
        // Further split each coordinate pair
        for (const auto& pair : coord_pairs) {

            vector<string> coords;
            string_utils::strsplit(pair, coords, ',');
            REQUIRE(coords.size() == 2);
        }
    }
    
    SECTION("Template split with maze builder context") {
        // Example: processing algorithm names separated by pipes
        string algos = "dfs|binary_tree|sidewinder";
        vector<string> algo_list;
        
        string_utils::strsplit(algos, algo_list, '|');
        
        REQUIRE(algo_list.size() == 3);
        REQUIRE(algo_list[0] == "dfs");
        REQUIRE(algo_list[1] == "binary_tree");
        REQUIRE(algo_list[2] == "sidewinder");
        
        // Verify these are valid algorithm names that could be used with the maze builder
        for (const auto& algo : algo_list) {
            REQUIRE_FALSE(algo.empty());
            REQUIRE(algo.find_first_not_of("abcdefghijklmnopqrstuvwxyz_") == string::npos);
        }
    }
}

TEST_CASE("string_utils format wrapper functions", "[string_utils_format]") {

    using namespace mazes;
    using namespace std;

    SECTION("Format with single int argument") {
        string result = string_utils::format("{}", 42);
        REQUIRE(result == "42");
    }

    SECTION("Format with single float argument") {
        string result = string_utils::format("{:.2f}", 3.14159f);
        REQUIRE(result == "3.14");
    }

    SECTION("Format with two int arguments") {
        string result = string_utils::format("{}, {}", 10, 20);
        REQUIRE(result == "10, 20");
    }

    SECTION("Format with int and float arguments") {
        string result = string_utils::format("Value: {}, Rate: {:.1f}", 100, 2.5f);
        REQUIRE(result == "Value: 100, Rate: 2.5");
    }

    SECTION("Format with two float arguments") {
        string result = string_utils::format("x: {:.1f}, y: {:.1f}", 1.2f, 3.4f);
        REQUIRE(result == "x: 1.2, y: 3.4");
    }

    SECTION("Format with string_view format string") {
        string_view format_str = "Number: {}";
        string result = string_utils::format(format_str, 999);
        REQUIRE(result == "Number: 999");
    }

    SECTION("Format with const char* format string") {
        const char* format_str = "Float: {:.3f}";
        string result = string_utils::format(format_str, 2.71828f);
        REQUIRE(result == "Float: 2.718");
    }

    SECTION("Format with complex format string") {
        string result = string_utils::format("Coords: ({}, {}), Distance: {:.2f}", 5, 10, 7.07f);
        REQUIRE(result == "Coords: (5, 10), Distance: 7.07");
    }

    SECTION("Format with zero arguments") {
        string result = string_utils::format("Hello World"sv);
        REQUIRE(result == "Hello World");
    }

    SECTION("Format with lvalue references") {
        int x = 42;
        float y = 3.14f;
        string result = string_utils::format("x={}, y={:.1f}", x, y);
        REQUIRE(result == "x=42, y=3.1");
    }
}

TEST_CASE("string_utils format wrapper edge cases", "[string_utils_format_edge]") {

    using namespace mazes;
    using namespace std;

    SECTION("Format with empty format string") {
        string result = string_utils::format(""sv);
        REQUIRE(result == "");
    }

    SECTION("Format with special characters") {
        string result = string_utils::format("Special: {}", 123);
        REQUIRE(result == "Special: 123");
    }

    SECTION("Format with negative numbers") {
        string result = string_utils::format("{}, {:.1f}", -42, -3.14f);
        REQUIRE(result == "-42, -3.1");
    }

    SECTION("Format with zero values") {
        string result = string_utils::format("{}, {:.1f}", 0, 0.0f);
        REQUIRE(result == "0, 0.0");
    }

    SECTION("Format with large numbers") {
        string result = string_utils::format("{}, {:.0f}", 1000000, 1234567.89f);
        REQUIRE(result == "1000000, 1234568");
    }
}
