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
#include "grid.h"
#include "grid_interface.h"

namespace mazes {
	class distances;
	class cell;

	class distance_grid : public grid {

	public:
		explicit distance_grid(unsigned int width, unsigned int length, unsigned int height = 0);
		
		void set_distances(std::shared_ptr<distances> d) noexcept;
		std::shared_ptr<distances> get_distances() const noexcept;
        const std::unique_ptr<grid>& get_grid() const noexcept;
		
        virtual unsigned int get_rows() const noexcept override;
        virtual unsigned int get_columns() const noexcept override;
        virtual std::vector<std::uint8_t> to_pixels(const unsigned int cell_size = 25) const noexcept override;
        virtual std::vector<std::shared_ptr<cell>> to_vec() const noexcept override;

        virtual void append(std::unique_ptr<grid_interface> const& other_grid) noexcept override;
        virtual void insert(std::shared_ptr<cell> const& parent, int index) noexcept override;
        virtual bool update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept override;
        virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, int index) const noexcept override;
        virtual void del(std::shared_ptr<cell> parent, int index) noexcept override;

        virtual std::shared_ptr<cell> get_root() const noexcept override;

        // Grid tostring method
        friend std::ostream& operator<<(std::ostream& os, distance_grid& dg) {
            // First sort cells by row then column
            std::vector<std::shared_ptr<cell>> cells;
            cells.reserve(dg.get_rows() * dg.get_columns());
            // populate the cells with the BST
            dg.sort(dg.get_root(), ref(cells));
            dg.sort_by_row_then_col(ref(cells));

            // ---+
            static constexpr auto barrier = { MAZE_BARRIER2, MAZE_BARRIER2, MAZE_BARRIER2, MAZE_CORNER };
            static const std::string wall_plus_corner{ barrier };
            std::stringstream output;
            output << MAZE_CORNER;
            for (auto i{ 0u }; i < dg.get_columns(); i++) {
                output << wall_plus_corner;
            }
            output << "\n";

            auto rowCounter{ 0u }, columnCounter{ 0u };
            while (rowCounter < dg.get_rows()) {
                std::stringstream top_builder, bottom_builder;
                top_builder << MAZE_BARRIER1;
                bottom_builder << MAZE_CORNER;
                while (columnCounter < dg.get_columns()) {
                    auto next_index{ rowCounter * dg.get_columns() + columnCounter };
                    if (next_index < cells.size()) {
                        auto&& temp = cells.at(next_index);
                        // bottom left cell needs boundaries
                        if (temp == nullptr)
                            temp = { std::make_shared<cell>(-1, -1, next_index) };
                        std::string body = " " + dg.contents_of(temp).value_or(" ") + " ";
                        static const std::string vertical_barrier_str{ MAZE_BARRIER1 };
                        std::string east_boundary = temp->is_linked(temp->get_east()) ? " " : vertical_barrier_str;
                        top_builder << body << east_boundary;
                        std::string south_boundary = temp->is_linked(temp->get_south()) ? "   " : wall_plus_corner.substr(0, wall_plus_corner.size() - 1);
                        bottom_builder << south_boundary << "+";
                        columnCounter++;
                    }
                }
                columnCounter = 0;
                rowCounter++;
                output << top_builder.str() << "\n" << bottom_builder.str() << "\n";
            } // while
            os << output.str() << std::endl;

            return os;
        } // << operator
    protected:
        virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
        virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

	private:
		std::shared_ptr<distances> m_distances;

		std::optional<std::string> to_base64(int value) const;
	};
}

#endif // DISTANCE_GRID_H