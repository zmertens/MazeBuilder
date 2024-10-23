/**
 * Basic program to test the maze_builder library in JavaScript
 */

#include <iostream>
#include <exception>
#include <vector>
#include <list>
#include <string>
#include <cstdlib>
#include <random>
#include <functional>
#include <memory>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/args_builder.h>
#include <MazeBuilder/output_types_enum.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_builder.h>
#include <MazeBuilder/writer.h>
#include <MazeBuilder/maze_builder.h>

class maze {
private:
    int m_rows;
    int m_cols;
    int m_depth;
    int m_seed;
    std::string m_algorithm;
    std::string m_output;
public:
    maze(int r, int c, int d, int s, const std::string& algorithm, const std::string& str)
        : m_rows(r), m_cols(c), m_depth(d), m_seed(s), m_algorithm(algorithm), m_output(str) {
            mazes::maze_types my_maze_type = mazes::maze_types::BINARY_TREE;
            
            std::mt19937 rng_engine{ static_cast<unsigned long>(m_seed) };
            auto get_int = [&rng_engine](auto low, auto high) {
                std::uniform_int_distribution<int> dist {low, high};
                return dist(rng_engine);
            };

            mazes::maze_builder builder;
            auto my_maze = builder.rows(r).columns(c).height(d)
                .seed(s).rng(rng_engine).get_int(get_int)
                .block_type(-1).show_distances(true)
                .maze_type(my_maze_type).build();

            m_output = my_maze.to_str();
    }

    std::string get_output() const {
        return m_output;
    }
    std::string get_algorithm() const {
        return m_algorithm;
    }
    int get_rows() const {
        return m_rows;
    }
    int get_cols() const {
        return m_cols;
    }
    int get_depth() const {
        return m_depth;
    }
    int get_seed() const {
        return m_seed;
    }

    static std::shared_ptr<maze> get_instance(int r, int c, int d, int s, const std::string& algo, const std::string& str) {
        static std::shared_ptr<maze> inst = std::make_shared<maze>(r, c, d, s, std::cref(algo), std::cref(str));
        return inst;
    }

};

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_module) {
    emscripten::class_<maze>("maze")
        .smart_ptr<std::shared_ptr<maze>>("std::shared_ptr<maze>")
        .constructor<int, int, int, int, const std::string&, const std::string&>()
        .property("output", &maze::get_output)
        .property("algorithm", &maze::get_algorithm)
        .property("rows", &maze::get_rows)
        .property("cols", &maze::get_cols)
        .property("depth", &maze::get_depth)
        .class_function("get_instance", &maze::get_instance, emscripten::allow_raw_pointers());
}
#endif

int main(int argc, char* argv[]) {

    using namespace std;

    try {
        // pass
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
