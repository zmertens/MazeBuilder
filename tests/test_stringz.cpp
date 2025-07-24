#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <list>
#include <memory>
#include <random>
#include <type_traits>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE("string_view_utils::split works correctly", "[string_view_utils]") {

    SECTION("Split with bracket delimiter") {

        std::string_view example = "-d[0:-1]";
        auto result = string_view_utils::split(example, "[");
        
        REQUIRE(result.size() == 2);
        auto it = result.begin();
        REQUIRE(*it == "-d");
        ++it;
        REQUIRE(*it == "0:-1]");
    }
    
    SECTION("Split with default space delimiter") {

        std::string_view example = "hello world test";
        auto result = string_view_utils::split(example);
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == "hello");
        ++it;
        REQUIRE(*it == "world");
        ++it;
        REQUIRE(*it == "test");
    }
    
    SECTION("Split with custom delimiter") {
        std::string_view example = "one,two,three";
        auto result = string_view_utils::split(example, ",");
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == "one");
        ++it;
        REQUIRE(*it == "two");
        ++it;
        REQUIRE(*it == "three");
    }
    
    SECTION("Split with multi-character delimiter") {
        std::string_view example = "one::two::three";
        auto result = string_view_utils::split(example, "::");
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == "one");
        ++it;
        REQUIRE(*it == "two");
        ++it;
        REQUIRE(*it == "three");
    }
    
    SECTION("Split empty string") {
        std::string_view example = "";
        auto result = string_view_utils::split(example, ",");
        
        REQUIRE(result.empty());
    }
    
    SECTION("Split with no delimiter found") {
        std::string_view example = "nodlimiterhere";
        auto result = string_view_utils::split(example, ",");
        
        REQUIRE(result.size() == 1);
        REQUIRE(*result.begin() == "nodlimiterhere");
    }
}

TEST_CASE( "Benchmark stringz ops ", "[benchmark stringz]" ) {

    static constexpr auto ROWS = 50, COLUMNS = 50, LEVELS = 10;
    static constexpr auto SEED = 12345;
    static constexpr auto ALGO = algo::DFS;

    // BENCHMARK("Benchmark stringz::stringify") {
    //    auto maze_opt = factory::create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS)._algo(ALGO).seed(SEED));

    //    REQUIRE(maze_opt);

    //    auto s = stringz::stringify(cref(maze_opt));

    //    REQUIRE(!s.empty());
    // };

    //BENCHMARK("Benchmark stringz::objectify") {

    //    auto maze_opt = factory::create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS)._algo(ALGO).seed(SEED));

    //    REQUIRE(maze_opt.has_value());

    //    auto s = stringz::stringify(cref(maze_opt.value()));

    //    vector<tuple<int, int, int, int>> vertices;

    //    vector<vector<uint32_t>> faces;

    //    stringz::objectify(cref(maze_opt.value()), vertices, faces, s);

    //    REQUIRE(!vertices.empty());

    //    REQUIRE(!faces.empty());
    //};
}

