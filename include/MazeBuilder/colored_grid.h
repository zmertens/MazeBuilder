#ifndef COLORED_GRID_H
#define COLORED_GRID_H

#include <memory>
#include <cstdint>
#include <vector>
#include <optional>

#include <MazeBuilder/grid.h>

namespace mazes {

    class cell;
    class distance_grid;

	class colored_grid : public grid {
    
        public:
    
        friend class binary_tree;
        friend class dfs;
        friend class sidewinder;

        explicit colored_grid(unsigned int rows, unsigned int cols, unsigned int height = 1u);

        virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
        virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;
	
        private:
    
        std::shared_ptr<distance_grid> m_distance_grid;
	};

}
#endif // COLORED_GRID_H
