#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <sstream>
#include <algorithm>
#include <functional>
#include <random>
#include <future>
#include <thread>

#include "maze_algo_interface.h"
#include "grid.h"
#include "binary_tree.h"
#include "sidewinder.h"

using namespace std;
using namespace mazes;

static auto get_int = [](int low, int high) ->int {
    random_device rd;
    seed_seq seed {rd()};
    mt19937 mt(seed);
    uniform_int_distribution<int> dist {low, high};
    return dist(mt);
};

// static auto append_grids = [](grid_ptr& g1, grid_ptr& g2, grid_ptr& g12) -> void {
    // g12.reset();
    // g12 = {make_unique<grid>(g1->get_rows() * g2->get_rows(), g1->get_columns() * g2->get_columns())};
// };

TEST_CASE("Compare maze algos", "[maze_algos_race]") {
    grid_ptr _grid_from_bt {make_unique<grid>(500, 500)};
    grid_ptr _grid_from_sw {make_unique<grid>(500, 500)};

    auto&& future_grid_from_bt = std::async(std::launch::async, [&] {
        mazes::binary_tree bt;
        return bt.run(_grid_from_bt, get_int, false);
    });
    auto&& future_grid_from_sw = std::async(std::launch::async, [&] {
        mazes::sidewinder sw;
        return sw.run(_grid_from_sw, get_int, false);
    });

    // BENCHMARK("benchmark binary_tree algo") {
    //     return future_grid_from_bt.get();
    // };

    // BENCHMARK("benchmark sidewinder algo") {
    //     return future_grid_from_sw.get();
    // };

    SECTION("Check for success", "[assert_section]]") {
        auto&& success = future_grid_from_bt.get();
        REQUIRE(success == true);
        stringstream ss;
        ss << *_grid_from_bt.get();
        REQUIRE(ss.str().length() != 0);
        success = future_grid_from_sw.get();
        REQUIRE(success == true);
        ss.clear();
        ss << *_grid_from_sw.get();
        REQUIRE(ss.str().length() != 0);
    }
}

TEST_CASE("Cells have neighbors", "[cells]") {

    using namespace std;

    auto is_equal = [&](shared_ptr<cell> cell1, shared_ptr<cell> cell2) -> bool {
        return cell1 == cell2;
    };

    // cell1 has cell2 neighbor to the south
    shared_ptr<cell> cell1 {make_shared<cell>(0, 0)};
    shared_ptr<cell> cell2 {make_shared<cell>(0, 1)};

    SECTION("Cell has neighbor to south") {
        cell1->set_south(cell2);
        REQUIRE(is_equal(cell1->get_south(), cell2));
        auto&& neighbors = cell1->get_neighbors();
        REQUIRE(!neighbors.empty());
    }

    SECTION("Cells are linked") {
        // links are bi-directional by default
        cell1->link(cell1, cell2);
        REQUIRE(cell1->is_linked(cell2));
    }
}

TEST_CASE("Threading grids", "[threading]") {
    grid_ptr _grid1 {make_unique<grid>(250, 250)};
    grid_ptr _grid2 {make_unique<grid>(250, 250)};
    grid_ptr _grid3 {make_unique<grid>(250, 250)};
    grid_ptr _grid4 {make_unique<grid>(250, 250)};
    
    auto run_bt = [&](grid_ptr& g)->bool {
        binary_tree bt;
        return bt.run(ref(g), get_int, false);
    };
    
    // thread t1 (run_bt, _grid1);
    // thread t2 (run_bt, _grid2);
    // thread t3 (run_bt, _grid3);
    // thread t4 (run_bt, _grid4);
    // t1.join();
    // t2.join();
    // t3.join();
    // t4.join();

    // grid_ptr _grid12;
    // append_grids(_grid1, _grid2, _grid12);
    // grid_ptr _grid34; = move(append_grids(_grid3, _grid4));
    // grid_ptr _grid_final = move(append_grids(_grid12, _grid34));

    // stringstream ss;
    // ss << *_grid12.get();
    // REQUIRE(ss.str().empty() == false);
    // REQUIRE(_grid12->get_rows() >= _grid1->get_rows());
    // REQUIRE(_grid12->get_rows() >= _grid2->get_rows());
    // REQUIRE(_grid12->get_columns() >= _grid1->get_columns());
    // REQUIRE(_grid12->get_columns() >= _grid2->get_columns());
}
