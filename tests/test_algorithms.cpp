#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <vector>
#include <sstream>
#include <algorithm>
#include <functional>
#include <random>
#include <future>
#include <chrono>
#include <thread>
#include <mutex>

#include "maze_algo_interface.h"
#include "cell.h"
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

TEST_CASE("Make a very large grid", "[large grid]") {
    unique_ptr<grid> very_large_grid{ make_unique<grid>(1'000, 2) };
    binary_tree bt_algo;
    REQUIRE(bt_algo.run(ref(very_large_grid), get_int, false));
}

TEST_CASE("Searching the grid yields positive results", "[search]") {
    unsigned int rows {25}, columns {20};
    vector<unsigned int> shuffled_ints {rows * columns};
    shuffled_ints.reserve(rows * columns);
    int next_index {0};
    for (auto itr {shuffled_ints.begin()}; itr != shuffled_ints.end(); itr++) {
        *itr = next_index++;
    }

    auto rd = std::random_device{};
    auto rng = std::default_random_engine{ rd() };
    shuffle(begin(shuffled_ints), end(shuffled_ints), rng);

    unique_ptr<grid> _grid {make_unique<grid>(rows, columns)};
    
    for (auto&& i : shuffled_ints) {
        auto&& found = _grid->search(_grid->get_root(), i);
        REQUIRE(found != nullptr);
    }
}

TEST_CASE("Compare maze algos", "[compare successes]") {
    unique_ptr<grid> _grid_from_bt {make_unique<grid>(50, 50)};
    unique_ptr<grid> _grid_from_sw {make_unique<grid>(49, 49)};

    auto&& future_grid_from_bt = std::async(std::launch::async, [&] {
        mazes::binary_tree bt;
        return bt.run(_grid_from_bt, get_int, false);
    });
    auto&& future_grid_from_sw = std::async(std::launch::async, [&] {
        mazes::sidewinder sw;
        return sw.run(_grid_from_sw, get_int, false);
    });

    SECTION("Check for success", "[assert_section]") {
        auto start = chrono::system_clock::now();
        auto&& success = future_grid_from_bt.get();
        auto elapsed1 = chrono::system_clock::now() - start;
        REQUIRE(success);
        // program should not take zero seconds to run
        auto elapsed1ms {chrono::duration_cast<chrono::milliseconds>(elapsed1).count()};
        REQUIRE(elapsed1ms != 0);
        stringstream ss;
        ss << *_grid_from_bt.get();
        REQUIRE(ss.str().length() != 0);
        start = chrono::system_clock::now();
        success = future_grid_from_sw.get();
        auto elapsed2 = chrono::system_clock::now() - start;
        REQUIRE(success == true);
        auto elapsed2ms {chrono::duration_cast<chrono::milliseconds>(elapsed2).count()};
        ss.clear();
        ss << *_grid_from_sw.get();
        REQUIRE(ss.str().length() != 0);
        // curious if either algo runs in identical time
        REQUIRE(elapsed1ms != elapsed2ms);
    }
}

TEST_CASE("Cells have neighbors", "[cells]") {

    // cell1 has cell2 neighbor to the south
    shared_ptr<cell> cell1 {make_shared<cell>(0, 0, 0)};
    shared_ptr<cell> cell2 {make_shared<cell>(0, 1, 1)};

    SECTION("Cell has neighbor to south") {
        cell1->set_south(cell2);
        REQUIRE(cell1->get_south() == cell2);
        auto&& neighbors = cell1->get_neighbors();
        REQUIRE(!neighbors.empty());
    }

    SECTION("Cells are linked") {
        // links are bi-directional by default
        cell1->link(cell1, cell2);
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
    }
}

TEST_CASE("Grids are sortable", "[sort]") {
    unique_ptr<grid> g1 {make_unique<grid>(100, 100)};
    vector<shared_ptr<cell>> sorted_cells;
    g1->sort(g1->get_root(), sorted_cells);
    // each sorted cell should increase in index value up until the max in the grid
    unsigned int max {0};
    for (auto&& cell : sorted_cells) {
        REQUIRE(cell->get_index() >= max);
        max = cell->get_index();
    }
}

TEST_CASE("Packaged task grids", "[packaged tasks]") {
    unique_ptr<grid> _grid1 {make_unique<grid>(250, 250)};
    unique_ptr<grid> _grid2 {make_unique<grid>(250, 250)};
    unique_ptr<grid> _grid3 {make_unique<grid>(250, 250)};
    unique_ptr<grid> _grid4 {make_unique<grid>(250, 250)};
    
    auto run_bt = [&](unique_ptr<grid>& g)->bool {
        binary_tree bt;
        return bt.run(ref(g), get_int, false);
    };
    
    packaged_task<bool(unique_ptr<grid>&)> task1 (run_bt);
    packaged_task<bool(unique_ptr<grid>&)> task2 (run_bt);
    packaged_task<bool(unique_ptr<grid>&)> task3 (run_bt);
    packaged_task<bool(unique_ptr<grid>&)> task4 (run_bt);

    auto fut1 = task1.get_future();
    auto fut2 = task2.get_future();
    auto fut3 = task3.get_future();
    auto fut4 = task4.get_future();

    // execute the packaged tasks
    task1(ref(_grid1));
    task2(ref(_grid2));
    task3(ref(_grid3));
    task4(ref(_grid4));

    // check results
    REQUIRE(fut1.get() == true);
    REQUIRE(fut2.get() == true);
    REQUIRE(fut3.get() == true);
    REQUIRE(fut4.get() == true);
} // packaged tasks

TEST_CASE("Threading mazes and appending together", "[threading mazes]") {
    auto a1 = {0, 1, 2, 3, 4, 5};
    auto a2 = {6, 7, 8, 9, 10};

    vector<unsigned int> increments;

    mutex mtx;

    // count asynchrously
    auto increments_by_1 = std::thread([&]()->void {
        lock_guard<mutex> lock {mtx};
        for (auto&& i : a1) {
            increments.emplace_back(i + 1);
        }
    });
    auto increments_by_2 = std::thread([&]()->void {
        lock_guard<mutex> lock {mtx};
        for (auto&& i : a2) {
            increments.emplace_back(i + 2);
        }
    });

    increments_by_1.join();
    increments_by_2.join();

    for (auto&& i : increments)
        REQUIRE(i);
}
