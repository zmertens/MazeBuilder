#ifndef PATHTRACER_APP_H
#define PATHTRACER_APP_H

#include <memory>
#include <string>
#include <string_view>

#include <MazeBuilder/algo_interface.h>
#include <MazeBuilder/singleton_base.h>

namespace mazes
{
    class grid_interface;
    class randomizer;
}

/// @brief 3D maze path-tracing demo using SDL3 GPU API
///
/// Generates a 2D maze with MazeBuilder, converts the wall cells into spheres
/// arranged in 3D space, then software-ray-traces the scene and uploads the
/// resulting pixel buffer to the GPU via SDL_GPU for display.
class pathtracer_app : public mazes::algo_interface,
                       public mazes::singleton_base<pathtracer_app>
{
    friend class singleton_base;

public:
    pathtracer_app(std::string_view title, std::string_view version, int w, int h);

    pathtracer_app(const std::string& title, const std::string& version, int w, int h);

    ~pathtracer_app() override;

    bool run(mazes::grid_interface* g, mazes::randomizer& rng) const noexcept override;

private:
    struct pathtracer_impl;
    std::unique_ptr<pathtracer_impl> m_impl;
};

#endif // PATHTRACER_APP_H
