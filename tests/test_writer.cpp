
#include <vector>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "file_types_enum.h"
#include "writer.h"

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

TEST_CASE("Writer can produce a data format for output", "[reads data]") {

}