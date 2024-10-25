#include <random>
#include <vector>
#include <string>
#include <memory>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/output_types_enum.h>
#include <MazeBuilder/writer.h>
#include <MazeBuilder/maze_types_enum.h>
#include <MazeBuilder/maze_builder.h>

using namespace std;
using namespace mazes;

TEST_CASE("Writer can receive program arguments", "[determine output format]") {
	writer my_writer;
	
	vector<string> good_filenames{ "1.txt", "2.txt", "3.txt", "1.obj", "2.obj", "7.png" };

	vector<string> bad_filenames{ "1.text", "2.plain_text", "3.plain_txt", "1.object", "2.objobj", "3obj", "a.ping", ""};

	for (auto&& good : good_filenames) {
		auto&& ftype = my_writer.get_output_type(good);
		CHECK_FALSE(ftype == output_types::UNKNOWN);
	}

	for (auto&& bad : bad_filenames) {
		auto&& ftype = my_writer.get_output_type(bad);
		CHECK(ftype == output_types::UNKNOWN);
	}

	REQUIRE(my_writer.get_output_type("stdout") == output_types::STDOUT);
}

TEST_CASE("Writer can produce a PNG file", "[does png]") {
	mt19937 rng{ random_device{}() };
	auto get_int = [&rng](auto x, auto y)->auto {
		return uniform_int_distribution<int>{x, y}(rng);
		};

    maze_builder builder;
    auto my_maze = builder.rows(10).columns(10).height(10).get_int(get_int).rng(rng).build();
	
	auto&& my_png = my_maze->to_pixels(15);

	REQUIRE(!my_png.empty());

	writer my_writer;

	REQUIRE(my_writer.get_output_type("1.png") == output_types::PNG);

	REQUIRE(my_writer.write_png("1.png", my_png, my_maze->rows * 4, my_maze->columns * 4));
}
