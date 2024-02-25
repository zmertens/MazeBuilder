#include "maze_builder_impl.h"

#include <memory>
#include <exception>

#include "ibuilder.h"
#include "craft.h"

maze_builder_impl::maze_builder_impl(const std::string& description)
: m_description(description) {

}

void maze_builder_impl::seed(unsigned int s) {
    this->s = s;
}

void maze_builder_impl::interactive(bool i) {
    this->is_interactive = i;
}

void maze_builder_impl::algo(const std::string& algo) {
    this->algorithm = algo;
}

void maze_builder_impl::output(const std::string& filename) {
    this->filename = filename;
}

/**
 * @param seed defaults to zero
*/
ibuilder::imaze_ptr maze_builder_impl::build() {
    using namespace std;

    if (this->is_interactive) {
        imaze_ptr my_maze (new craft(m_description, this->s));
        return my_maze;
    } else {
        // throw std::runtime_error("failed to build maze");
    }
}