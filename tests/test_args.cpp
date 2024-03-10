#include <unordered_map>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include "../sources/args_builder.h"

using namespace std;
using namespace mazes;

TEST_CASE( "Args are computed", "[args]" ) {
    std::unordered_map<std::string, std::string> args = {
        {"algorithm", "sidewinder"},
        {"seed", "0"},
        {"interactive", "0"},
        {"output", "stdout"},
        {"help", "help"},
        {"version", "version"}
    };

    args_builder args_builder {args};

    REQUIRE(args == args_builder.build());
    REQUIRE(args["algorithm"].compare("sidewinder") == 0);
    REQUIRE(args_builder.get_seed() == 0);
    REQUIRE(args_builder.is_interactive() == false);
    REQUIRE(args["output"].compare("stdout") == 0);
}

// TEST_CASE("Benchmarking Factorials") {
//     BENCHMARK("Factorial(20)") {
//         return Factorial(20);
//     };
//     BENCHMARK("Factorial(25)") {
//         return Factorial(25);
//     };
//     BENCHMARK("Factorial(42)") {
//         return Factorial(42);
//     };    
// }
