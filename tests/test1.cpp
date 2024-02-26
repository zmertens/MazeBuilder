#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "../sources/imaze.h"
#include "../sources/bst_maze.h"

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

TEST_CASE("BST Algo returns success", "[bst_algo]") {
    imaze::imaze_ptr my_maze (new bst_maze("bst algo", 1337, "stdout"));
    bool success = my_maze->run();
    REQUIRE(success == true);
}
