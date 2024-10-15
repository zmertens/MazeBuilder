/**
 * @file distances.h
 * @brief Distances class header file - Computes distances between cells in a maze
 * @ingroup Maze
 * 
*/

#ifndef DISTANCES_H
#define DISTANCES_H

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>

namespace mazes {

    class cell;

    class distances {
    public:
        explicit distances(std::shared_ptr<cell> root);

		int& operator[](const std::shared_ptr<cell>& cell) noexcept {
			return m_cells[cell];
		}

        void set(std::shared_ptr<cell> cell, int distance) noexcept;

        bool contains(const std::shared_ptr<cell>& cell) const noexcept;

        std::shared_ptr<distances> path_to(std::shared_ptr<cell> goal) const noexcept;
        std::pair<std::shared_ptr<cell>, int> max() const noexcept;

        void collect_keys(std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    private:
        std::shared_ptr<cell> m_root;
        std::unordered_map<std::shared_ptr<cell>, int> m_cells;
    };

} // namespace mazes
#endif // DISTANCES_H