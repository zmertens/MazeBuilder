#ifndef WAVEFRONT_OBJECT_HELPER
#define WAVEFRONT_OBJECT_HELPER

#include <MazeBuilder/algo_interface.h>

/// @file wavefront_object_helper.h
/// @namespace mazes
namespace mazes
{
    /// @class wavefront_object_helper
    /// @brief Transform a maze into a Wavefront object string
    class wavefront_object_helper : public algo_interface
    {
    public:
        /// @brief Run the wavefront object helper algorithm
        /// @param g The grid to process
        /// @param rng The randomizer to use
        /// @return True if successful, false otherwise
        virtual bool run(grid_interface *g, randomizer &rng) const noexcept override;
    };

}

#endif // WAVEFRONT_OBJECT_HELPER
