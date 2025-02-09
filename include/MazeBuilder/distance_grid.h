/**
 *	@brief Derived class that computes distance between cells in a maze
 *
 */

#ifndef DISTANCE_GRID_H
#define DISTANCE_GRID_H

#include <string>
#include <unordered_map>
#include <memory>

#include <MazeBuilder/grid.h>

namespace mazes {

	class distances;
	class cell;

	class distance_grid : public grid {

	public:
        friend class binary_tree;
        friend class dfs;
        friend class sidewinder;


		explicit distance_grid(unsigned int width, unsigned int length, unsigned int height = 1u);
		
        virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
        virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

        std::shared_ptr<distances> get_distances() const noexcept;
	private:
        std::shared_ptr<grid_interface> m_grid;
		std::shared_ptr<distances> m_distances;

		std::optional<std::string> to_base36(int value) const;
        void calc_distances() noexcept;
	};
}

#endif // DISTANCE_GRID_H
