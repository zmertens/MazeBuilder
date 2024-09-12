#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <string>
#include <memory>
#include <cstdlib>

namespace mazes {
	class cell;
	class grid;
	class grid_interface {
	public:
		virtual const std::unique_ptr<grid>& get_grid() const noexcept = 0;
		virtual std::shared_ptr<cell> get_root() const noexcept = 0;
		virtual std::string contents_of(const std::shared_ptr<cell>& c) const noexcept = 0;
		virtual std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept = 0;
	};
}

#endif // GRID_INTERFACE_H
