#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <vector>
#include <string>
#include <ostream>
#include <sstream>
#include <memory>
#include <cstdint>
#include <optional>
#include <tuple>

#include <MazeBuilder/enums.h>
#include <MazeBuilder/cell.h>

namespace mazes {
	
    class grid_interface {
	
        public:
        /// @brief Get dimensions of grid, no assumptions about rows, columns, or height ordering
        virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept = 0;
		
        /// @brief Transformation functions
        virtual void to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept = 0;	
        virtual void to_vec2(std::vector<std::vector<std::shared_ptr<cell>>>& cells) const noexcept = 0;

		/// @brief Get detailed information of a cell in the grid
		/// @param c 
		/// @return 
		virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept = 0;
		virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept = 0;

        protected:

        /// @brief Provide a string representation of the grid
        /// @param os 
        /// @param g 
        /// @return 
        friend std::ostream& operator<<(std::ostream& os, const grid_interface& g) {
            auto [rows, columns, height] = g.get_dimensions();

            // First sort cells by row then column
            std::vector<std::shared_ptr<cell>> cells;
            cells.reserve(rows * columns);

            // populate the cells from the grid
            g.to_vec(cells);

            // ---+
            static constexpr auto barrier = { BARRIER2, BARRIER2, BARRIER2, BARRIER2, BARRIER2, CORNER };
            static const std::string wall_plus_corner{ barrier };
            
            std::stringstream output;
            output << CORNER;
            
            for (auto i{ 0u }; i < columns; i++) {
                output << wall_plus_corner;
            }
            output << "\n";

            auto rowCounter{ 0u }, columnCounter{ 0u };
            while (rowCounter < rows) {
                std::stringstream top_builder, bottom_builder;
                top_builder << BARRIER1;
                bottom_builder << CORNER;
                
                while (columnCounter < columns) {

                    auto next_index{ rowCounter * columns + columnCounter };

                    if (next_index < cells.size()) {
                        auto&& temp = cells.at(next_index);
                        
                        // Bottom left cell needs boundaries
                        if (temp == nullptr)
                            temp = { std::make_shared<cell>(-1, -1, next_index) };
                        // 5 spaces in body for single-digit number to hold base36 values
                        static const std::string vertical_barrier_str{ BARRIER1 };
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
                            os << "No contents for cell at index " << next_index << "\n";
                        }

                        columnCounter++;
                    }
                }

                columnCounter = 0;
                rowCounter++;

                output << top_builder.str() << "\n" << bottom_builder.str() << "\n";
            } // while

            os << output.str() << "\n";

            return os;
        } // << operator
	};
}

#endif // GRID_INTERFACE_H
