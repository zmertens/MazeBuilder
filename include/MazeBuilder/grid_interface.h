#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <cstdint>
#include <optional>

#include "maze_types_enum.h"
#include "cell.h"

namespace mazes {

	class grid_interface {
	public:
        virtual ~grid_interface() = default;
		virtual unsigned int get_rows() const noexcept = 0;
		virtual unsigned int get_columns() const noexcept = 0;
        virtual unsigned int get_height() const noexcept = 0;
		virtual void append(std::shared_ptr<grid_interface> const& other_grid) noexcept = 0;
		virtual void insert(std::shared_ptr<cell> const& parent, int index) noexcept = 0;
		virtual bool update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept = 0;
		virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, int index) const noexcept = 0;
		virtual void del(std::shared_ptr<cell> parent, int index) noexcept = 0;
		virtual std::shared_ptr<cell> get_root() const noexcept = 0;
        virtual void preorder(std::vector<std::shared_ptr<cell>>& cells) const noexcept = 0;    
        virtual void populate_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept = 0;	
        virtual void make_sorted_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept = 0;
		virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept = 0;
		virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept = 0;
    protected:
		friend std::ostream& operator<<(std::ostream& os, grid_interface& g) {
            // First sort cells by row then column
            std::vector<std::shared_ptr<cell>> cells;
            cells.reserve(g.get_rows() * g.get_columns());
            // populate the cells with the BST
            g.make_sorted_vec(cells);

            // ---+
            static constexpr auto barrier = { MAZE_BARRIER2, MAZE_BARRIER2, MAZE_BARRIER2, MAZE_BARRIER2, MAZE_BARRIER2, MAZE_CORNER };
            static const std::string wall_plus_corner{ barrier };
            std::stringstream output;
            output << MAZE_CORNER;
            for (auto i{ 0u }; i < g.get_columns(); i++) {
                output << wall_plus_corner;
            }
            output << "\n";

            auto rowCounter{ 0u }, columnCounter{ 0u };
            while (rowCounter < g.get_rows()) {
                std::stringstream top_builder, bottom_builder;
                top_builder << MAZE_BARRIER1;
                bottom_builder << MAZE_CORNER;
                while (columnCounter < g.get_columns()) {
                    auto next_index{ rowCounter * g.get_columns() + columnCounter };
                    if (next_index < cells.size()) {
                        auto&& temp = cells.at(next_index);
                        // bottom left cell needs boundaries
                        if (temp == nullptr)
                            temp = { std::make_shared<cell>(-1, -1, next_index) };
                        // 5 spaces in body for single-digit number to hold base36 values
                        static const std::string vertical_barrier_str{ MAZE_BARRIER1 };
                        auto has_contents_val = g.contents_of(std::cref(temp)).has_value();
                        if (has_contents_val) {
                            auto val = g.contents_of(std::cref(temp)).value_or(" ");
                            std::string body = "";
                            switch (val.size()) {
                            case 1: body = "  " + val + "  "; break;
                            case 2: body = " " + val + "  "; break;
                            case 3: body = " " + val + " "; break;
                            case 4: body = " " + val; break;
                                // case 1 is default
                            default: body = "  " + val + "  "; break;
                            }
                            auto east_boundary = temp->is_linked(temp->get_east()) ? " " : vertical_barrier_str;
                            auto south_boundary = temp->is_linked(temp->get_south()) ? "     " : wall_plus_corner.substr(0, wall_plus_corner.size() - 1);
                            top_builder << body << east_boundary;
                            bottom_builder << south_boundary << "+";
                        } else {
                            os << "No contents for cell at index " << next_index << std::endl;
                        }
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
	};
}

#endif // GRID_INTERFACE_H
