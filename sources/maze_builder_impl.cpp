#include "maze_builder_impl.h"

#include "craft.h"

maze_builder_impl::maze_builder_impl(const args_handler& args)
: args(args) {

}

/**
 * @param seed defaults to zero
*/
bool maze_builder_impl::build(unsigned int seed) {
    bool success = false;
    success = true;

    if (this->args.interactive) {
        craft craft_engine ("craft-sdl3", seed);
        craft_engine.run();
    }

    return success;
}