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

static shared_ptr<grid> my_grid = make_shared<grid>(10, 10, 10);

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

TEST_CASE("Test distance grid", "[distance grid]") {
    static constexpr auto ROW = 10, COL = 10, HEIGHT = 10;
    unique_ptr<grid_interface> my_distance_grid = make_unique<distance_grid>(ROW, COL, HEIGHT);

    my_distance_grid->insert(my_distance_grid->get_root(), 0);
	my_distance_grid->insert(my_distance_grid->get_root(), ROW + COL * (ROW - 1));

	auto&& start = my_distance_grid->search(my_distance_grid->get_root(), 0);
    auto&& end = my_distance_grid->search(my_distance_grid->get_root(), 1);

    REQUIRE(start != end);

    mt19937 rng{ 42681ul };
    auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
    };
	binary_tree bt_algo;
	REQUIRE(bt_algo.run(ref(my_distance_grid), cref(get_int), cref(rng)));

    auto dists = make_shared<distances>(my_distance_grid->get_root());

	dynamic_cast<distance_grid*>(my_distance_grid.get())->set_distances(dists);

	//auto&& path = start->distances()->path_to(end);

	//REQUIRE(path);
	//REQUIRE(path->get_cells().size() >= 2);

	//if (auto db = dynamic_cast<distance_grid*>(my_distance_grid.get()); db != nullptr) {
	//	auto&& dg = db->get_distances();
	//	REQUIRE(dg->operator[](end) == 1);
 //       stringstream ss;
	//	ss << db;
 //       REQUIRE(!ss.str().empty());
	//}
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
