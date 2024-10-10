/**
 *	@brief Derived class that computes distance between cells in a maze
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
	class cell;

	class distance_grid : public grid_interface {

	public:
		explicit distance_grid(unsigned int width, unsigned int length, unsigned int height = 0);
		
        virtual unsigned int get_rows() const noexcept override;
        virtual unsigned int get_columns() const noexcept override;
        virtual std::vector<std::uint8_t> to_pixels(const unsigned int cell_size = 3) const noexcept override;
        virtual void make_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept override;

        virtual void append(std::unique_ptr<grid_interface> const& other_grid) noexcept override;
        virtual void insert(std::shared_ptr<cell> const& parent, int index) noexcept override;
        virtual bool update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept override;
        virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, int index) const noexcept override;
        virtual void del(std::shared_ptr<cell> parent, int index) noexcept override;

        virtual std::shared_ptr<cell> get_root() const noexcept override;
        virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
        virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

        void calc_distances() noexcept;
	private:
        std::unique_ptr<grid_interface> m_grid;
		std::shared_ptr<distances> m_distances;
		std::optional<std::string> to_base36(int value) const;
	};
}

#endif // DISTANCE_GRID_H
