#include <vector>
#include <string>
#include <memory>
#include <iosfwd>
#include <fstream>
#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/writer.h>

using namespace std;
using namespace mazes;

TEST_CASE("Writer can process good text file names", "[good text filenames]") {
	writer my_writer;

    // Good file names that the writer can determine what type to write per the extension
	vector<string> good_filenames{ "1.txt", "1.obj", ".obj", ".txt", ".png", "my.jpg", "other.jpeg" };

	for (const auto& gf : good_filenames) {
        REQUIRE(my_writer.write(gf, "data"));
	}
}

TEST_CASE("Writer can process bad file names", "[bad filenames]") {
    writer my_writer;

    vector<string> bad_filenames{ "1.text", "2.plain_text", "3.plain_txt", "1.object", "2.objobj", "3obj", "a.ping", "" };

    for (const auto& bf : bad_filenames) {
        REQUIRE_FALSE(my_writer.write(bf, "data"));
    }
}

TEST_CASE("writer::write writes data to file successfully", "[writer writes]") {
    mazes::writer w;
    std::string filename = "test_file.txt";
    std::string data = "Hello, world!";

    REQUIRE_NOTHROW(w.write(filename, data));

    // Verify the file contents
    std::ifstream f1 (filename);
    REQUIRE(f1.is_open());
    std::string f1_content((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
    REQUIRE(f1_content == data);

    // Clean up
    f1.close();
    std::remove(filename.c_str());
}

TEST_CASE("Writer writes data to stdout successfully", "[writer verifies writes]") {
    mazes::writer w;
    std::string data = "Hello, world!";
    std::ostringstream oss;

    REQUIRE_NOTHROW(w.write(ref(oss), data));

    REQUIRE(oss.str() == data + "\n");
}

