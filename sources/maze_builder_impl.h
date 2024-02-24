#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

#include "args_handler.h"
#include "imaze.h"

class args_handler;

class maze_builder_impl : public imaze {
public:
    maze_builder_impl(const args_handler& args);
    bool build(unsigned int seed = 0) override;
private:
    const args_handler& args;
};

#endif // MAZE_BUILDER_H