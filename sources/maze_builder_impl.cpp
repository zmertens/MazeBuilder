#include "maze_builder_impl.h"

#include <memory>
#include <stdexcept>
#include <random>

#include "imaze_builder.h"
#include "bst.h"
#include "grid.h"
#include "cell.h"
#include "craft.h"

maze_builder_impl::maze_builder_impl(const std::string& description)
: m_description(description)
, s(0)
, is_interactive(false)
, algorithm("bst")
, filename("stdout") {

}

maze_builder_impl& maze_builder_impl::seed(unsigned int s) {
    this->s = s;
    return *this;
}

maze_builder_impl& maze_builder_impl::interactive(bool i) {
    this->is_interactive = i;
    return *this;
}

maze_builder_impl& maze_builder_impl::algo(const std::string& algo) {
    this->algorithm = algo;
    return *this;
}

maze_builder_impl& maze_builder_impl::output(const std::string& filename) {
    this->filename = filename;
    return *this;
}

imaze::imaze_ptr maze_builder_impl::build() {
    using namespace std;

    if (this->is_interactive) {
        imaze::imaze_ptr my_maze (new craft(m_description, this->s));
        return my_maze;
    } else {
        auto get_int = [=](int low, int high) -> int {
            random_device rd;
            seed_seq seed {rd()};
            mt19937 rng_engine {seed};
            uniform_int_distribution<int> dist {low, high};
            return dist(rng_engine);
        };
        if (this->algorithm.compare("bst") == 0) {
            grid g {5, 5};
            // imaze::imaze_ptr my_maze {make_unique<bst>()};
            // return my_maze;
            return nullptr;
        } else {
            throw runtime_error("Invalid algorithm: " + this->algorithm);
        }
    }
}