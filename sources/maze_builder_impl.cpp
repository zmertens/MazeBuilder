#include "maze_builder_impl.h"

#include <memory>
#include <exception>

#include "ibuilder.h"
#include "bst_maze.h"
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

ibuilder::imaze_ptr maze_builder_impl::build() {
    using namespace std;

    if (this->is_interactive) {
        imaze_ptr my_maze (new craft(m_description, this->s));
        return my_maze;
    } else {
        if (this->algorithm.compare("bst") == 0) {
            imaze_ptr my_maze (new bst_maze(m_description, this->s, this->filename));
            return my_maze;
        } else {
            throw runtime_error("Invalid algorithm: " + this->algorithm);
        }
    }
}