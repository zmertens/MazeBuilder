#include <random>
#include <vector>
#include <string>
#include <memory>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/writer.h>
#include <MazeBuilder/maze_builder.h>

using namespace std;
using namespace mazes;

TEST_CASE("Writer can receive program arguments", "[determine output format]") {
	writer my_writer;
	
	vector<string> good_filenames{ "1.txt", "2.txt", "3.txt", "1.obj", "2.obj", "7.png" };

	vector<string> bad_filenames{ "1.text", "2.plain_text", "3.plain_txt", "1.object", "2.objobj", "3obj", "a.ping", ""};

	for (auto&& good : good_filenames) {
		auto&& ftype = my_writer.get_output_type(good);
		CHECK_FALSE(ftype == outputs::TOTAL);
	}

	for (auto&& bad : bad_filenames) {
		auto&& ftype = my_writer.get_output_type(bad);
		CHECK(ftype == outputs::TOTAL);
	}

	REQUIRE(my_writer.get_output_type("stdout") == outputs::STDOUT);
}

TEST_CASE("Writer can produce a PNG file", "[does png]") {
    // mazes::builder builder;
    // unique_ptr<maze> my_maze = builder.rows(10).columns(10).height(10).build();
	// progress p{};
	// computations::compute_geometry(cref(my_maze));
	// auto elapsed = p.elapsed_s();
	// auto&& my_png = my_maze->to_pixels(15);

	// REQUIRE(!my_png.empty());

	// writer my_writer;

	// REQUIRE(my_writer.get_output_type("1.png") == outputs::PNG);

	// REQUIRE(my_writer.write_png("1.png", my_png, my_maze->rows * 4, my_maze->columns * 4));
}
