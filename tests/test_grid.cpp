#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <sstream>
#include <random>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "binary_tree.h"
#include "grid.h"
#include "grid_interface.h"
#include "distance_grid.h"
#include "distances.h"
#include "maze_factory.h"

using namespace mazes;
using namespace std;

enum class WorkerState {
	IDLE,
	WORKING,
	DONE
}; 

struct WorkerItem {
    bool load = false;
    unique_ptr<grid_interface> grid;
};

struct Worker {
    mutex mtx;
    condition_variable cnd;
    WorkerItem item;
    WorkerState state = WorkerState::IDLE;
    bool should_stop = false;
};

static unique_ptr<grid_interface> my_grid = make_unique<grid>(10, 10, 10);

TEST_CASE( "Test grid init", "[init]" ) {
    REQUIRE(my_grid->get_rows() == 10);
    REQUIRE(my_grid->get_columns() == 10);
}

TEST_CASE( "Test grid insertion and search", "[insert and search]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), my_cell->get_index());
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE( "Test grid deletion ", "[delete]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), my_cell->get_index());
    my_grid->del(my_grid->get_root(), 0);
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result != my_cell);
}

TEST_CASE( "Test grid update ", "[update]" ) {
	auto my_cell = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(my_cell);
    auto&& modify_this_index = my_cell->get_index();
    modify_this_index = my_grid->get_columns() - 1;
	bool success = my_grid->update(my_cell, my_cell->get_index(), modify_this_index);
	REQUIRE(success);
	auto result = my_grid->search(my_grid->get_root(), modify_this_index);
    REQUIRE(result != nullptr);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE( "Test appending grids", "[append]") {
    unique_ptr<grid_interface> my_grid2 = make_unique<grid>(10, 10, 10);
    my_grid2->insert(my_grid2->get_root(), 0);
    my_grid2->insert(my_grid2->get_root(), 1);
    my_grid2->insert(my_grid2->get_root(), 2);
    my_grid->append(my_grid2);

    REQUIRE(my_grid->search(my_grid->get_root(), 0) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 1) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 2) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 3) != nullptr);
}

TEST_CASE("Test distance grid", "[distance grid output]") {
	auto my_distance_grid = make_unique<distance_grid>(10, 10, 10);
	auto my_cell = make_shared<cell>(0, 0, 0);
	my_distance_grid->insert(my_distance_grid->get_root(), my_cell->get_index());
	auto result = my_distance_grid->search(my_distance_grid->get_root(), 0);
	REQUIRE(result->get_index() == my_cell->get_index());
	stringstream ss;
	ss << *my_distance_grid;
	REQUIRE(ss.str().size() > 0);
}

TEST_CASE("Test cells", "[distance cells]") {
    // Create cells
    auto cell1 = make_shared<cell>(0, 0, 1);
    auto cell2 = make_shared<cell>(0, 1, 2);
    auto cell3 = make_shared<cell>(1, 0, 3);
    auto cell4 = make_shared<cell>(1, 1, 4);

    // Link cells
    cell1->link(cell1, cell2);
    cell2->link(cell2, cell4);
    cell1->link(cell1, cell3);
    cell3->link(cell3, cell4);

    // Get distances from cell1
    auto distances = cell1->get_distances_from(cell1);

    REQUIRE(distances);

    SECTION("Distance from root to itself is zero") {
        REQUIRE(distances->operator[](cell1) == 0);
    }
    
    SECTION("Distance from root to adjacent cells") {
        REQUIRE(distances->operator[](cell2) == 1);
        REQUIRE(distances->operator[](cell3) == 1);
    }
    
    SECTION("Distance from root to diagonal cell") {
        REQUIRE(distances->operator[](cell4) == 2);
    }
    
    SECTION("Path to a specific cell") {
        auto path = distances->path_to(cell4);
        REQUIRE(path->operator[](cell4) == 2);
        REQUIRE(path->operator[](cell2) == 1);
        REQUIRE(path->operator[](cell1) == 0);
    }
    
    SECTION("Maximum distance from root") {
        auto [max_cell, max_distance] = distances->max();
        REQUIRE(max_distance == 2);
        REQUIRE((max_cell == cell4 || max_cell == cell3 || max_cell == cell2));
    }
}

TEST_CASE("Test paths", "[distances and paths]") {
    random_device rd;
    std::mt19937 rng{ rd() };
    auto get_int = [&rng](auto low, auto high)->auto {
		uniform_int_distribution<int> dist{ low, high };
		return dist(rng);
		};
    bool success = mazes::maze_factory::gen_maze(maze_types::BINARY_TREE, ref(my_grid), cref(get_int), cref(rng));
    REQUIRE(success);
	auto distances = my_grid->get_root()->get_distances_from(my_grid->get_root());
	REQUIRE(distances);
    auto center = my_grid->search(my_grid->get_root(), my_grid->get_columns() / 2);
    REQUIRE(center);
	auto path_to_center = distances->path_to(center);
    REQUIRE(path_to_center);
    REQUIRE(path_to_center->max().second > 0);
}

TEST_CASE("Test multiple grids", "[multiple grids]") {
    mt19937 rng{ 42681ul };
    auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };
    binary_tree bt_algo;

    auto worker_run = [&rng, &get_int, &bt_algo](auto&& worker) {
        while (1) {
            while (worker->state != WorkerState::WORKING && !worker->should_stop) {
                std::unique_lock<std::mutex> u_lck(worker->mtx);
                worker->cnd.wait(u_lck);
            }
            if (worker->should_stop) {
                worker->state = WorkerState::DONE;
                break;
            }

            WorkerItem* worker_item = &worker->item;
            if (worker_item->load) {
                // generate respective portion of maze with indices (p, q)
				bool success = bt_algo.run(ref(worker_item->grid), cref(get_int), cref(rng));
            }
            // This line would compute any OpenGL components of the maze
            // this->compute_maze(worker_item); // Assuming this is a member function

            worker->mtx.lock();
            worker->state = WorkerState::DONE;
            worker->mtx.unlock();
        }
        return 0;
        };

    const int grid_size = 100;
    const int num_workers = 4;
	const int subgrid_size = grid_size / num_workers;

    vector<thread> threads;
	vector<Worker> workers(num_workers);
	for (int i = 0; i < num_workers; ++i) {
        workers.at(i).state = WorkerState::WORKING;
		workers.at(i).should_stop = false;
        workers.at(i).item.load = true;
		workers.at(i).item.grid = make_unique<grid>(subgrid_size, subgrid_size, subgrid_size);
		threads.push_back(thread(worker_run, &workers[i]));
	}

	for (int i = 0; i < num_workers; ++i) {
		workers[i].mtx.lock();
        // Do work
        workers[i].state = WorkerState::WORKING;
        workers[i].should_stop = true;
		workers[i].mtx.unlock();
		workers[i].cnd.notify_one();
	}

	for (int i = 0; i < num_workers; ++i) {
		threads[i].join();
	}

	for (int i = 0; i < num_workers; ++i) {
		REQUIRE(workers[i].state == WorkerState::DONE);
		my_grid->append(cref(workers[i].item.grid));
	}

	// REQUIRE(my_grid->get_rows() * my_grid->get_columns() > grid_size * grid_size);
}
