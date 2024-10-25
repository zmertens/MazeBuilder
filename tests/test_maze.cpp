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

    SECTION("Create a 100x100 maze") {
        static constexpr auto NUM_ROWS = 100, NUM_COLS = 100, HEIGHT = 10;
        static constexpr auto OFFSET_X = 10, OFFSET_Z = 10;
        maze_builder builder;
        auto maze1 = builder.rows(NUM_ROWS).columns(NUM_COLS).height(HEIGHT)
            .offset_x(OFFSET_X).offset_z(OFFSET_Z)
            .get_int(get_int).rng(rng).build();
        REQUIRE(maze1->columns == NUM_COLS);
        REQUIRE(maze1->rows == NUM_ROWS);
        REQUIRE(maze1->height == HEIGHT);
        REQUIRE(maze1->offset_x == OFFSET_X);
        REQUIRE(maze1->offset_z == OFFSET_Z);
        REQUIRE(maze1->get_progress_in_seconds() != 0.0);
    }

    BENCHMARK("Create a 100x100 maze") {
        static constexpr auto NUM_ROWS = 100, NUM_COLS = 100, HEIGHT = 10;
        static constexpr auto OFFSET_X = 10, OFFSET_Z = 10;
        maze_builder builder;
        auto maze1 = builder.rows(NUM_ROWS).columns(NUM_COLS).height(HEIGHT)
            .offset_x(OFFSET_X).offset_z(OFFSET_Z)
            .get_int(get_int).rng(rng).build();
        REQUIRE(maze1->columns == NUM_COLS);
        REQUIRE(maze1->rows == NUM_ROWS);
        REQUIRE(maze1->height == HEIGHT);
        REQUIRE(maze1->offset_x == OFFSET_X);
        REQUIRE(maze1->offset_z == OFFSET_Z);
        REQUIRE(maze1->get_progress_in_seconds() != 0.0);
    };
}

TEST_CASE( "Test mazes", "[maze progress]") {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };
    maze_builder builder;
    SECTION("BINARY_TREE PROGRESS") {
        auto maze1 = builder.rows(10).columns(10).height(10)
            .get_int(get_int).rng(rng).maze_type(maze_types::BINARY_TREE)
            .offset_x(10)
            .offset_z(10).build();
        REQUIRE(maze1->get_progress_in_seconds() <= 10.0);
        REQUIRE(maze1->get_progress_in_ms() <= 1.0);
    }
    SECTION("SIDEWINDER PROGRESS") {
        auto maze1 = builder.rows(10).columns(10).height(10)
            .get_int(get_int).rng(rng).maze_type(maze_types::SIDEWINDER)
            .offset_x(10)
            .offset_z(10).build();
        REQUIRE(maze1->get_progress_in_seconds() <= 10.0);
        REQUIRE(maze1->get_progress_in_ms() <= 1.0);
    }

    SECTION("DFS PROGRESS") {
        auto maze1 = builder.rows(50).columns(50).height(50)
            .get_int(get_int).rng(rng).maze_type(maze_types::DFS)
            .offset_x(10)
            .offset_z(10).build();
        REQUIRE(maze1->get_progress_in_seconds() <= 10.0);
        REQUIRE(maze1->get_progress_in_ms() <= 1.0);
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

