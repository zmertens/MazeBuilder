#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <vector>
#include <string>
#include <memory>
#include <cstdlib>

namespace mazes {
	class cell;
	class grid;
	class grid_interface {
	public:
		virtual unsigned int get_rows() const noexcept = 0;
		virtual unsigned int get_columns() const noexcept = 0;
		virtual void append(std::unique_ptr<grid> const& other_grid) noexcept = 0;
		virtual void insert(std::shared_ptr<cell> const& parent, unsigned int index) noexcept = 0;
		virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, unsigned int index) const noexcept = 0;
		virtual void del(std::shared_ptr<cell> parent, unsigned int index) noexcept = 0;
		virtual std::shared_ptr<cell> get_root() const noexcept = 0;
		virtual std::string contents_of(const std::shared_ptr<cell>& c) const noexcept = 0;
		virtual std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept = 0;
		virtual std::vector<std::uint8_t> to_png(const unsigned int cell_size = 25) const noexcept = 0;
	};
}

#endif // GRID_INTERFACE_H
