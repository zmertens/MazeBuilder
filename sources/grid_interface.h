#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <memory>
#include <cstdlib>

namespace mazes {

	class cell;

	class grid_interface {
	private:
		virtual std::uint16_t contents_of(const std::shared_ptr<cell>& c) const noexcept = 0;
		virtual std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept = 0;
	};
}

#endif // GRID_INTERFACE_H
