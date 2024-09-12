/**
 *	@brief Utility class to compute distance between cells in a maze
 *
 */

#ifndef DISTANCE_GRID_H
#define DISTANCE_GRID_H

#include <string>
#include <unordered_map>
#include <memory>

#include "cell.h"
#include "grid_interface.h"


namespace mazes {
	class distances;
	class grid;
	class cell;

	class distance_grid : public grid_interface {

	public:
		explicit distance_grid(unsigned int width, unsigned int length, unsigned int height = 0);
		
		virtual const std::unique_ptr<grid>& get_grid() const noexcept override;
		virtual std::shared_ptr<cell> get_root() const noexcept override;
		virtual std::string contents_of(const std::shared_ptr<cell>& c) const noexcept override;
		virtual std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept override;		
	private:
		std::shared_ptr<distances> m_distances;
		std::unique_ptr<grid> m_grid;

		std::string to_base64(int value) const;
	};
}

#endif // DISTANCE_GRID_H