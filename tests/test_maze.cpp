#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>

#include "grid.h"

using namespace mazes;
using namespace std;

static shared_ptr<grid> my_grid = make_shared<grid>(10, 10, 10);

TEST_CASE( "Test grid init", "[init]" ) {
    REQUIRE(my_grid->get_rows() == 10);
    REQUIRE(my_grid->get_columns() == 10);
}
