#include <catch2/catch_test_macros.hpp>

#include "../sources/maze_builder_impl.h"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}

TEST_CASE("BST Algo returns success", "[bst_algo]") {
    maze_builder_impl bst_builder {"bst_algo_maze"};
    auto bst_maze = bst_builder.seed(1337).build();
    bool success = bst_maze->run();
    REQUIRE(success == true);
}