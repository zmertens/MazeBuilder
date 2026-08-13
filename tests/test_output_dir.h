#ifndef TEST_OUTPUT_DIR_H
#define TEST_OUTPUT_DIR_H

#include <filesystem>
#include <string_view>

struct test_output_dir
{
    std::filesystem::path path;

    explicit test_output_dir(std::string_view name)
        : path(std::filesystem::temp_directory_path() / "MB" / name)
    {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~test_output_dir()
    {
        std::filesystem::remove_all(path);
    }

    test_output_dir(const test_output_dir&) = delete;
    test_output_dir& operator=(const test_output_dir&) = delete;
    test_output_dir(test_output_dir&&) = delete;
    test_output_dir& operator=(test_output_dir&&) = delete;
};

#endif // TEST_OUTPUT_DIR_H
