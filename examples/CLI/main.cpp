#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <iomanip>

#include <MazeBuilder/maze_builder.h>

std::string maze_builder_version = "maze_builder\nversion\t" + mazes::VERSION;

static constexpr auto MAZE_BUILDER_HELP = R"help(
        Usages: maze_builder.exe [OPTION(S)]... [OUTPUT]
        Generates mazes and exports to different formats
        Options: case-sensitive, long options must use '=' combination
        Example: maze_builder.exe -r 10 -c 10 -a binary_tree > out_maze.txt
        Example: mb.exe --rows=10 --columns=10 --algo=dfs -o out_maze.txt_
          -a, --algo         dfs, sidewinder, binary_tree [default]
          -c, --columns      columns
          -d, --distances    show distances using base36 numbers
          -e, --encode       encode maze to base64 string
          -h, --help         display this help message
          -j, --json         run with arguments in JSON format
          -s, --seed         seed for the mt19937 generator
          -r, --rows         rows
          -o, --output       [txt|text] [jpg|jpeg] [png] [obj|object] [stdout]
          -v, --version      display program version
    )help";

int main(int argc, char* argv[]) {

    using namespace std;

#if defined(MAZE_DEBUG)
    maze_builder_version += " - DEBUG";
#endif

    // Copy command arguments and skip the program name
    vector<string> args_vec{ argv + 1, argv + argc };

    mazes::args maze_args{ };
    if (!maze_args.parse(args_vec)) {
        cerr << "Invalid arguments, parsing failed." << endl;
        return EXIT_FAILURE;
    }

    if (maze_args.get("-h").has_value() || maze_args.get("--help").has_value()) {
        cout << MAZE_BUILDER_HELP << endl;
        return EXIT_SUCCESS;
    }

    if (maze_args.get("-v").has_value() || maze_args.get("--version").has_value()) {
        cout << maze_builder_version << endl;
        return EXIT_SUCCESS;
    }

    auto seed = 0;
    if (maze_args.get("-s").has_value()) {
        seed = stoi(maze_args.get("-s").value());
    } else if (maze_args.get("--seed").has_value()) {
        seed = stoi(maze_args.get("--seed").value());
    }

    std::mt19937 rng_engine{ static_cast<unsigned long>(seed)};
    auto get_int = [&rng_engine](int low, int high) -> int {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    string algo_str = "dfs";
    if (maze_args.get("-a").has_value()) {
        algo_str = maze_args.get("-a").value();
    } else if (maze_args.get("--algo").has_value()) {
        algo_str = maze_args.get("--algo").value();
    }

    auto rows = 3, columns = 2, levels = 1;
    if (maze_args.get("-c").has_value()) {
        columns = stoi(maze_args.get("-c").value());
    } else if (maze_args.get("--columns").has_value()) {
        columns = stoi(maze_args.get("--columns").value());
    }

    if (maze_args.get("-r").has_value()) {
        rows = stoi(maze_args.get("-r").value());
    } else if (maze_args.get("--rows").has_value()) {
        rows = stoi(maze_args.get("--rows").value());
    }

    if (maze_args.get("-l").has_value()) {
        levels = stoi(maze_args.get("-l").value());
    } else if (maze_args.get("--levels").has_value()) {
        levels = stoi(maze_args.get("--levels").value());
    }

    bool distances{ false };
    if (maze_args.get("-d").has_value()) {
        distances = true;
    } else if (maze_args.get("--distances").has_value()) {
        distances = true;
    }

    bool encodes{ false };
    if (maze_args.get("-e").has_value()) {
        encodes = true;
    } else if (maze_args.get("--encodes").has_value()) {
        encodes = true;
    }

    string output_file_str = "stdout";
    if (maze_args.get("-o").has_value()) {
        output_file_str = maze_args.get("-o").value();
    } else if (maze_args.get("--output").has_value()) {
        output_file_str = maze_args.get("--output").value();
    }

    // Run the command-line program

    try {
        
        bool success = false;

        mazes::algo my_maze_type = mazes::to_algo_from_string(cref(algo_str));
        mazes::output my_output_type = mazes::to_output_from_string(output_file_str.substr(output_file_str.find_last_of(".") + 1));

        static constexpr auto BLOCK_ID = -1;

        using maze_ptr = optional<unique_ptr<mazes::maze>>;

        mazes::progress<chrono::milliseconds, chrono::high_resolution_clock> clock;
        clock.start();

        maze_ptr next_maze_ptr = mazes::factory::create(
            mazes::configurator().columns(columns).rows(rows).levels(levels)
            .distances(distances).seed(seed)._algo(my_maze_type)
            ._output(my_output_type)
            .block_id(BLOCK_ID));

        auto dur = clock.elapsed<>();

        if (!next_maze_ptr.has_value()) {
            throw runtime_error("Failed to create maze");
        }

        auto temp_s = mazes::stringz::stringify(cref(next_maze_ptr.value()));

        mazes::base64_helper my_base64;
        auto maze_s = (encodes) ? my_base64.encode(cref(temp_s)) : temp_s;

        mazes::writer my_writer;

        switch (my_output_type) {
        case mazes::output::WAVEFRONT_OBJECT_FILE: {

            vector<tuple<int, int, int, int>> vertices;
            vector<vector<uint32_t>> faces;
            mazes::wavefront_object_helper woh{};
            auto obj_str = woh.to_wavefront_object_str(cref(vertices), cref(faces));
            success = my_writer.write_file(cref(output_file_str), cref(obj_str));
            break;
        }
        case mazes::output::PNG: {

            static constexpr auto STRIDE = 4u;
            vector<uint8_t> pixels;
            auto pixels_w{ 0 }, pixels_h{ 0 };
            pixels.reserve(rows * columns * STRIDE);
            if (distances) {
                mazes::stringz::to_pixels(cref(next_maze_ptr.value()), ref(pixels), ref(pixels_w), ref(pixels_h), STRIDE);
            } else {
                mazes::stringz::to_pixels(cref(maze_s), ref(pixels), ref(pixels_w), ref(pixels_h), STRIDE);
            }

            success = my_writer.write_png(cref(output_file_str), cref(pixels), pixels_w, pixels_h, STRIDE);
            break;
        }
        case mazes::output::JPEG: {

            static constexpr auto STRIDE = 4u;
            vector<uint8_t> pixels;
            auto pixels_w{ 0 }, pixels_h{ 0 };
            pixels.reserve(rows * columns * STRIDE);
            if (distances) {
                mazes::stringz::to_pixels(cref(next_maze_ptr.value()), ref(pixels), ref(pixels_w), ref(pixels_h), STRIDE);
            } else {
                mazes::stringz::to_pixels(cref(maze_s), ref(pixels), ref(pixels_w), ref(pixels_h), STRIDE);
            }

            success = my_writer.write_jpeg(cref(output_file_str), cref(pixels), pixels_w, pixels_h, STRIDE);
            break;
        }
        case mazes::output::JSON: {

            mazes::json_helper jh{};
            maze_args.set("duration", to_string(chrono::duration<double, milli>(dur).count()));
            maze_args.set("str", cref(maze_s));
            const auto& args = maze_args.get();
            const auto& args_to_json_str = jh.from(cref(args));
            success = my_writer.write_file(cref(output_file_str), cref(args_to_json_str));
            break;
        }
        case mazes::output::PLAIN_TEXT: {
            success = my_writer.write_file(cref(output_file_str), cref(maze_s));
            break;
        }
        case mazes::output::STDOUT: {

            cout << maze_s << endl;
            success = true;
            break;
        }
        default:
            success = false;
        }

        if (success) {
#if defined(MAZE_DEBUG)
            std::cout << "Writing to file: " << output_file_str << std::endl;
            std::cout << "Duration: " << fixed << setprecision(3) << dur << " milliseconds" << std::endl;
#endif
        }
        else {
            std::cerr << "Writing to: " << output_file_str << " failed!" << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
} // main
