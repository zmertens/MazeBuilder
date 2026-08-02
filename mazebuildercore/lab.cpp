#include <MazeBuilder/lab.h>

#include <MazeBuilder/cell.h>

#include <functional>

using namespace mazes;

/// @brief Create a bidirectional or unidirectional link between two cells
/// @param c1 First cell to link
/// @param c2 Second cell to link
/// @param bidi If true, creates bidirectional link; if false, only c1 links to c2
void lab::link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, const bool bidi) noexcept
{
    if (!c1 || !c2)
    {
        return;
    }

    // Add c2 to c1's links
    c1->add_link(c2);

    // If bidirectional, add c1 to c2's links
    if (bidi)
    {
        c2->add_link(c1);
    }
}

/// @brief Remove link between two cells
/// @param c1 First cell to unlink
/// @param c2 Second cell to unlink
/// @param bidi If true, removes bidirectional link; if false, only removes c1's link to c2
void lab::unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, const bool bidi) noexcept
{
    if (!c1 || !c2)
    {
        return;
    }

    // Remove c2 from c1's links
    c1->remove_link(c2);

    // If bidirectional, remove c1 from c2's links
    if (bidi)
    {
        c2->remove_link(c1);
    }
}
