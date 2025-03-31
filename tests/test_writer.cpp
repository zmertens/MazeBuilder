#include <vector>
#include <string>
#include <memory>
#include <iosfwd>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/writer.h>
#include <MazeBuilder/enums.h>

using namespace std;
using namespace mazes;

TEST_CASE("Writer can process good text file names", "[good text filenames]") {
	writer my_writer;

    // Good file names that the writer can determine what type to write per the extension
	vector<string> good_filenames{ "1.txt", "1.obj", ".object", ".text", ".png", "my.jpg", "other.jpeg" };

	for (const auto& gf : good_filenames) {
        REQUIRE(my_writer.write_file(gf, "data"));
	}
}

TEST_CASE("Writer can process bad file names", "[bad filenames]") {
    writer my_writer;

    vector<string> more_filenames{ "1-text", "2.plain_text", "3plain_txt", "4.objected", "5.objobj", "6obj", "a.ping", "pong" };

    for (const auto& more : more_filenames) {
        REQUIRE(my_writer.write_file(more, "data"));
    }

    for (auto bf : more_filenames) {
        auto bf_substr = bf.substr(bf.find_last_of('.') + 1);
        REQUIRE_THROWS_AS(mazes::to_output_from_string(bf_substr), std::invalid_argument);
    }
}

TEST_CASE("Writer writes data to file successfully", "[writer writes]") {
    mazes::writer w;
    std::string filename = "test_file.txt";
    std::string data = "Hello, world!";

    REQUIRE_NOTHROW(w.write_file(filename, data));

    // Verify the file contents
    std::ifstream f1 (filename);
    REQUIRE(f1.is_open());
    std::string f1_content((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
    REQUIRE(f1_content == data);

    // Clean up
    f1.close();
    std::remove(filename.c_str());
}

TEST_CASE("Writer writes data to stdout successfully", "[writer to stdout]") {
    mazes::writer w;
    std::string data = "Hello, world!";
    std::ostringstream oss;

    REQUIRE_NOTHROW(w.write(ref(oss), data));

    REQUIRE(oss.str() == data + "\n");
}

