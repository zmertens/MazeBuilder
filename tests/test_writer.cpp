
#include <vector>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "file_types_enum.h"
#include "writer.h"
#include "grid.h"

using namespace std;
using namespace mazes;

TEST_CASE("Writer can receive program arguments", "[determine output format]") {
	writer my_writer;
	
	vector<string> good_filenames{ "1.txt", "2.txt", "3.txt", "1.obj", "2.obj" };

	vector<string> bad_filenames{ "1.text", "2.plain_text", "3.plain_txt", "1.object", "2.objobj", "3obj", ""};

	for (auto&& good : good_filenames) {
		auto&& ftype = my_writer.get_filetype(good);
		CHECK_FALSE(ftype == file_types::UNKNOWN);
	}

	for (auto&& bad : bad_filenames) {
		auto&& ftype = my_writer.get_filetype(bad);
		CHECK(ftype == file_types::UNKNOWN);
	}

	REQUIRE(my_writer.get_filetype("stdout") == file_types::UNKNOWN);
}

TEST_CASE("Writer can produce a PNG file", "[does png]") {
	grid my_grid{ 100, 150 };
	auto&& my_png = my_grid.to_png();

	REQUIRE(!my_png.empty());

	writer my_writer;

	REQUIRE(my_writer.get_filetype("1.png") == file_types::PNG);

	//REQUIRE(my_writer.write("1.png", my_png, my_grid.get_rows(), my_grid.get_columns()));

}