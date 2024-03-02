#ifndef BST_HPP
#define BST_HPP

#include "maze_algo_interface.h"

class grid;

class bst : public maze_algo_interface {
public:

    bool run(grid& g, std::function<int(int, int)> const& get_int) const noexcept override;
private:

};

#endif // BST_HPP