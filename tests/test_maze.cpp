#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>

#include <MazeBuilder/maze_builder.h>
#include <MazeBuilder/maze_algo_interface.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>

using namespace mazes;
using namespace std;

TEST_CASE( "Test maze init", "[maze init]" ) {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    shared_ptr<maze_builder> maze1 = make_shared<maze_builder>(10, 10, 10);


    REQUIRE(maze1->get_columns() == 10);
    REQUIRE(maze1->get_rows() == 10);
}

TEST_CASE( "Test maze progress", "[maze progress]") {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    shared_ptr<maze_builder> maze1 = make_shared<maze_builder>(10, 10, 10);

    SECTION("Check no compute progress") {
        maze1->start_progress();
        maze1->stop_progress();
        REQUIRE(maze1->get_progress_in_seconds() <= 10.0);
        REQUIRE(maze1->get_progress_in_ms() <= 1.0);
    }

    SECTION("Check compute_str progresss") {
        maze1->start_progress();
        auto s = maze1->to_str();
        maze1->stop_progress();
        REQUIRE(maze1->get_progress_in_seconds() > 0);
    }
}

TEST_CASE( "Test maze compute geometry", "[maze geometry]") {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    unique_ptr<maze_builder> maze1 = make_unique<maze_builder>(10, 10, 10, maze_types::DFS, cref(get_int), cref(rng));
    REQUIRE(!maze1->to_wavefront_obj_str().empty());
    REQUIRE(maze1->get_vertices_size() > 0);
}

TEST_CASE("Make a very large grid", "[large grid]") {
    mt19937 rng{ 42681ul };
    static auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };
    unique_ptr<grid_interface> very_large_grid{ make_unique<grid>(1'000, 2) };
    binary_tree bt_algo;
    REQUIRE(bt_algo.run(ref(very_large_grid), cref(get_int), cref(rng)));
}

TEST_CASE("Searching the grid yields positive results", "[search]") {
    unsigned int rows{ 25 }, columns{ 20 };
    vector<unsigned int> shuffled_ints{ rows * columns };
    shuffled_ints.reserve(rows * columns);
    int next_index{ 0 };
    for (auto itr{ shuffled_ints.begin() }; itr != shuffled_ints.end(); itr++) {
        *itr = next_index++;
    }

    auto rd = std::random_device{};
    auto rng = std::default_random_engine{ rd() };
    shuffle(begin(shuffled_ints), end(shuffled_ints), rng);

    unique_ptr<grid> _grid{ make_unique<grid>(rows, columns) };

    for (auto&& i : shuffled_ints) {
        auto&& found = _grid->search(_grid->get_root(), i);
        REQUIRE(found != nullptr);
    }
}

TEST_CASE("Compare maze algos", "[compare successes]") {
    unique_ptr<grid_interface> _grid_from_bt{ make_unique<grid>(50, 150) };
    unique_ptr<grid_interface> _grid_from_sw{ make_unique<grid>(49, 29) };
    unique_ptr<grid_interface> _grid_from_dfs{ make_unique<grid>(50, 75) };

    mt19937 rng{ 42681ul };
    auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };

    auto&& future_grid_from_bt = std::async(std::launch::async, [&_grid_from_bt, &get_int, &rng] {
        mazes::binary_tree bt;
        return bt.run(ref(_grid_from_bt), get_int, rng);
        });
    auto&& future_grid_from_sw = std::async(std::launch::async, [&_grid_from_sw, &get_int, &rng] {
        mazes::sidewinder sw;
        return sw.run(ref(_grid_from_sw), get_int, rng);
        });
    auto&& future_grid_from_dfs = std::async(std::launch::async, [&_grid_from_dfs, &get_int, &rng] {
        mazes::dfs dfs;
        return dfs.run(ref(_grid_from_dfs), get_int, rng);
        });

    SECTION("Check for success", "[assert_section]") {
        auto start = chrono::system_clock::now();
        auto&& success = future_grid_from_bt.get();
        auto elapsed1 = chrono::system_clock::now() - start;
        REQUIRE(success);
        // program should not take zero seconds to run
        auto elapsed1ms{ chrono::duration_cast<chrono::milliseconds>(elapsed1).count() };
        REQUIRE(elapsed1ms != 0);
        stringstream ss;
        ss << *(dynamic_cast<grid*>(_grid_from_bt.get()));
        REQUIRE(ss.str().length() != 0);
        start = chrono::system_clock::now();
        success = future_grid_from_sw.get();
        auto elapsed2 = chrono::system_clock::now() - start;
        REQUIRE(success == true);
        auto elapsed2ms{ chrono::duration_cast<chrono::milliseconds>(elapsed2).count() };
        ss.clear();
        ss << *(dynamic_cast<grid*>(_grid_from_sw.get()));
        REQUIRE(ss.str().length() != 0);
        // curious if either algo runs in identical time
        REQUIRE(elapsed1ms != elapsed2ms);
    }
    SECTION("dfs success", "[assert dfs]") {
        auto start = chrono::system_clock::now();
        stringstream ss;
        ss << *(dynamic_cast<grid*>(_grid_from_dfs.get()));
        REQUIRE(ss.str().length() != 0);
        // curious if either algo runs in identical time
        REQUIRE(start != chrono::system_clock::now());
    }
}

TEST_CASE("Cells have neighbors", "[cells]") {

    // cell1 has cell2 neighbor to the south
    shared_ptr<cell> cell1{ make_shared<cell>(0, 0, 0) };
    shared_ptr<cell> cell2{ make_shared<cell>(0, 1, 1) };

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
    unique_ptr<grid> g1{ make_unique<grid>(100, 100) };
    vector<shared_ptr<cell>> sorted_cells;
    g1->sort(g1->get_root(), sorted_cells);
    // each sorted cell should increase in index value up until the max in the grid
    unsigned int max{ 0 };
    for (auto&& cell : sorted_cells) {
        REQUIRE(cell->get_index() >= max);
        max = cell->get_index();
    }
}

