#ifndef WORLD_H
#define WORLD_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "command_queue.h"

#include "map.h"
#include "resource_identifiers.h"

union SDL_Event;

std::string _check_for_gl_err(const char* file, int line) noexcept;

#define CHECK_GL_ERR() _check_for_gl_err(__FILE__, __LINE__)

using world_func = std::function<void(int, int, int, int, Map*)>;

class command_queue;
class player;
struct SDL_Window;

class world final {
public:
    explicit world(SDL_Window* window, font_manager& fonts, texture_manager& textures);

    void init() noexcept;

    void update(float dt);

    void draw() const noexcept;

    command_queue& get_command_queue() noexcept;

    // Destroy the world
    void destroy_world();

    void handle_event(SDL_Event& event);

    void set_player(player* player);

    void create_world(int p, int q, world_func func, Map *m, int chunk_size) const noexcept;

private:
    // Build the scene (initialize scene graph and layers)
    void build_scene();

    enum class Layer
    {
        PARALLAX_BACK = 0,
        PARALLAX_MID = 1,
        PARALLAX_FORE = 2,
        BACKGROUND = 3,
        FOREGROUND = 4,
        LAYER_COUNT = 5
    };

    static constexpr auto FORCE_DUE_TO_GRAVITY = -9.8f;

    SDL_Window* m_window;

    font_manager& m_fonts;
    texture_manager& m_textures;
    // SceneNode mSceneGraph;
    // std::array<SceneNode*, static_cast<std::size_t>(Layer::LAYER_COUNT)> mSceneLayers;

    command_queue m_command_queue;
    player* m_player;
};

#endif // WORLD_H
