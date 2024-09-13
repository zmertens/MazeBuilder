#ifndef COLORED_GRID_H
#define COLORED_GRID_H

#include "grid_interface.h"

#include <memory>
#include <cstdlib>
#include <vector>

namespace mazes {

    class distance_grid;
    class cell;
	class colored_grid : public grid_interface
	{
    public:
        explicit colored_grid(unsigned int width, unsigned int length, unsigned int height = 0);

        virtual unsigned int get_rows() const noexcept override;
        virtual unsigned int get_columns() const noexcept override;

        // Get bytewise representation of the grid
        virtual std::vector<std::uint8_t> to_png(const unsigned int cell_size = 25) const noexcept override;

        virtual void append(std::unique_ptr<grid> const& other_grid) noexcept override;
        virtual void insert(std::shared_ptr<cell> const& parent, unsigned int index) noexcept override;
        virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, unsigned int index) const noexcept override;
        virtual void del(std::shared_ptr<cell> parent, unsigned int index) noexcept override;

        virtual std::shared_ptr<cell> get_root() const noexcept override;

        virtual std::string contents_of(const std::shared_ptr<cell>& c) const noexcept override;
        virtual std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept override;
	private:
        std::unique_ptr<distance_grid> m_distance_grid;
	};

}
#endif // COLORED_GRID_H
