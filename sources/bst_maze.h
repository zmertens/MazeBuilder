#ifndef BST_MAZE_H
#define BST_MAZE_H

#include <string>

#include "imaze.h"
#include "output_writer.h"

class bst_maze : public imaze {
public:
    bst_maze(const std::string& desc, unsigned int seed, const std::string& out);
    bool run() override;
private:
    output_writer writer;

};

#endif // BST_MAZE_H