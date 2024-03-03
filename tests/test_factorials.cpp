#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

unsigned int Factorial( unsigned int number ) {
    return (number == 1 || number == 0) ? 1 : Factorial(number-1)*number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(0) == 1 );
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}

TEST_CASE("Benchmarking Factorials") {
    BENCHMARK("Factorial(20)") {
        return Factorial(20);
    };
    BENCHMARK("Factorial(25)") {
        return Factorial(25);
    };
    BENCHMARK("Factorial(42)") {
        return Factorial(42);
    };    
}
