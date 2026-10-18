/// @file main.cpp
/// @brief Amazing: shows a distance-gradient maze, balls roll toward its darker regions.
///        Press B to generate a new maze, Esc to quit.

#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    const std::string APP_NAME = "Amazing - " + std::string{mazes::buildinfo::VERSION};

    const std::filesystem::path TEMP_IMAGE_PATH{std::filesystem::temp_directory_path() / "amazing_maze_generated.png"};

    constexpr int MAX_ROWS_ROWS{40};
    constexpr int MAX_ROWS_COLUMNS{40};
    constexpr float FIXED_DELTA{1.0f / 120.0f};
    constexpr int SUB_STEPS{4};
    constexpr float PIXELS_PER_METER{20.0f};
    constexpr float GRAVITY_MPS2{1.0f};
    constexpr float GRADIENT_FORCE_N{1.5f};
    constexpr float BALL_LINEAR_DAMPING{1.2f};
    constexpr unsigned int MAX_WINDOW_DIM_PX{720u};
    // Matches the 12 px cell + 2 px wall layout written by the pixels create state
    constexpr int CELL_PITCH_PX{14};
    constexpr float PARALLAX_OFFSET_SCALE{5.5555512222333f};
    constexpr float STUCK_CHECK_SECONDS{0.75f};
    constexpr float STUCK_MIN_TRAVEL_M{0.15f};
    constexpr float JUMP_SPEED_MPS{4.0f};
    constexpr int JUMP_LOOKAHEAD_PX{CELL_PITCH_PX};

    /// @brief Draws the maze image but discards wall pixels, letting the parallax layer show through
    constexpr std::string_view WALL_CUTOUT_FRAG = R"(
uniform sampler2D texture;
uniform vec3 wall_color;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    if (distance(pixel.rgb, wall_color) < 0.01)
    {
        discard;
    }
    gl_FragColor = gl_Color * pixel;
})";

    /// @brief Random maze args. The distances flag makes the written image a distance gradient
    std::string generate_random_args(const mazes::randomizer &rng)
    {
        const auto algo = static_cast<mazes::algo>(rng.get_int(0, static_cast<int>(mazes::algo::TOTAL) - 1));
        auto s1{"-r "};
        auto s1_val{std::to_string(rng.get_int(5, MAX_ROWS_ROWS)) + " "};
        auto s2{"-c "};
        auto s2_val{std::to_string(rng.get_int(5, MAX_ROWS_COLUMNS)) + " "};
        auto s3{"-s "};
        auto s3_val{std::to_string(rng.get_int(1, rng.get_int(2, 100'000))) + " "};
        auto s4{"-a "};
        auto s4_val{std::string{mazes::to_sv_from_algo(algo)} + " "};
        auto s5{"--distances=[0:-1] "};
        auto s6{"-o "};
        auto s6_val{TEMP_IMAGE_PATH.string()};
        return s1 + s1_val + s2 + s2_val + s3 + s3_val + s4 + s4_val + s5 + s6 + s6_val;
    }
} // namespace

struct dynamic_ball
{
    static constexpr std::size_t COUNT = 24u;
    static constexpr float RADIUS_M = 0.15f;

    b2BodyId body{b2_nullBodyId};
    bool collected{false};
    int value{1};
    float lifetime_seconds{0.0f};
    float stuck_timer_seconds{0.0f};
    b2Vec2 last_checked_pos{0.0f, 0.0f};

    [[nodiscard]] bool is_positive() const noexcept { return value > 0; }
};

/// @brief Box2D world sized to the maze, stepped with a fixed delta so results repeat exactly
class ball_world
{
public:
    ball_world(const sf::Image &maze_image, const mazes::randomizer &rng)
        : image{maze_image}
    {
        b2WorldDef world_def = b2DefaultWorldDef();
        world_def.gravity = b2Vec2{0.0f, GRAVITY_MPS2};
        world_def.workerCount = 1;
        world = b2CreateWorld(&world_def);

        world_w = static_cast<float>(image.getSize().x) / PIXELS_PER_METER;
        world_h = static_cast<float>(image.getSize().y) / PIXELS_PER_METER;
        build_walls_from_image();
        solve_maze();

        balls.resize(dynamic_ball::COUNT);
        for (std::size_t i = 0; i < balls.size(); ++i)
        {
            const b2Vec2 spawn = pick_floor_position(rng);

            b2BodyDef body_def = b2DefaultBodyDef();
            body_def.type = b2_dynamicBody;
            body_def.linearDamping = BALL_LINEAR_DAMPING;
            body_def.position = spawn;
            const auto velocity = rng.sample_float_n(-1.0f, 1.0f, 2);
            body_def.linearVelocity = b2Vec2{velocity[0], velocity[1]};
            balls[i].body = b2CreateBody(world, &body_def);
            balls[i].last_checked_pos = spawn;
            balls[i].value = (i % 3u == 0u) ? -1 : 1;

            b2ShapeDef shape_def = b2DefaultShapeDef();
            shape_def.density = 1.0f;
            shape_def.material.friction = 0.2f;
            shape_def.material.restitution = 0.8f;
            const b2Circle circle{b2Vec2{0.0f, 0.0f}, dynamic_ball::RADIUS_M};
            b2CreateCircleShape(balls[i].body, &shape_def, &circle);
        }
    }

    ~ball_world()
    {
        if (b2World_IsValid(world))
        {
            b2DestroyWorld(world);
        }
    }

    ball_world(const ball_world &) = delete;
    ball_world &operator=(const ball_world &) = delete;

    void step(const int steps)
    {
        for (int i = 0; i < steps; ++i)
        {
            apply_gradient_descent();
            jump_stuck_balls();
            b2World_Step(world, FIXED_DELTA, SUB_STEPS);
            for (auto &ball : balls)
            {
                ball.lifetime_seconds += FIXED_DELTA;
            }
        }
    }

    [[nodiscard]] const std::vector<dynamic_ball> &get_balls() const noexcept { return balls; }
    [[nodiscard]] float get_world_width() const noexcept { return world_w; }
    [[nodiscard]] float get_world_height() const noexcept { return world_h; }

private:
    [[nodiscard]] bool is_wall(const int x, const int y) const
    {
        const auto size = image.getSize();
        if (x < 0 || y < 0 || x >= static_cast<int>(size.x) || y >= static_cast<int>(size.y))
        {
            return true;
        }
        return image.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}) == wall_color;
    }

    [[nodiscard]] float luminance_at(const int x, const int y) const
    {
        const sf::Color c = image.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)});
        return (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) / 255.0f;
    }

    /// @brief Central difference one cell pitch away, so flat distance cells still yield a direction.
    ///        Samples that land in a wall count as no change, so walls never attract or repel
    [[nodiscard]] b2Vec2 luminance_gradient(const int x, const int y) const
    {
        const float center = luminance_at(x, y);
        const auto sample = [&](const int sx, const int sy)
        { return is_wall(sx, sy) ? center : luminance_at(sx, sy); };
        return b2Vec2{sample(x + CELL_PITCH_PX, y) - sample(x - CELL_PITCH_PX, y),
                      sample(x, y + CELL_PITCH_PX) - sample(x, y - CELL_PITCH_PX)};
    }

    /// @brief Pushes each ball down the luminance gradient, toward darker parts of the image
    void apply_gradient_descent()
    {
        for (const auto &ball : balls)
        {
            const b2Vec2 pos = b2Body_GetPosition(ball.body);
            const int px = static_cast<int>(pos.x * PIXELS_PER_METER);
            const int py = static_cast<int>(pos.y * PIXELS_PER_METER);
            if (is_wall(px, py))
            {
                continue;
            }
            const b2Vec2 grad = luminance_gradient(px, py);
            const float len = std::sqrt(grad.x * grad.x + grad.y * grad.y);
            if (len > 1.0e-6f)
            {
                const b2Vec2 force{-grad.x / len * GRADIENT_FORCE_N, -grad.y / len * GRADIENT_FORCE_N};
                b2Body_ApplyForceToCenter(ball.body, force, true);
            }
        }
    }

    /// @brief Breadth-first flood from the darkest floor pixels (the gradient's goal) through every floor pixel,
    ///        giving the true path length to the goal that gradient descent alone can't see around dead ends
    void solve_maze()
    {
        const auto size = image.getSize();
        const int w = static_cast<int>(size.x);
        const int h = static_cast<int>(size.y);
        path_dist.assign(static_cast<std::size_t>(w) * h, -1);

        float darkest = 2.0f;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                if (!is_wall(x, y))
                {
                    darkest = std::min(darkest, luminance_at(x, y));
                }
            }
        }

        std::vector<int> queue;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                if (!is_wall(x, y) && luminance_at(x, y) <= darkest + 0.004f)
                {
                    path_dist[static_cast<std::size_t>(y) * w + x] = 0;
                    queue.push_back(y * w + x);
                }
            }
        }
        constexpr int DX[4]{1, -1, 0, 0};
        constexpr int DY[4]{0, 0, 1, -1};
        for (std::size_t head = 0; head < queue.size(); ++head)
        {
            const int cx = queue[head] % w;
            const int cy = queue[head] / w;
            const int d = path_dist[static_cast<std::size_t>(cy) * w + cx];
            for (int k = 0; k < 4; ++k)
            {
                const int nx = cx + DX[k];
                const int ny = cy + DY[k];
                if (is_wall(nx, ny) || path_dist[static_cast<std::size_t>(ny) * w + nx] >= 0)
                {
                    continue;
                }
                path_dist[static_cast<std::size_t>(ny) * w + nx] = d + 1;
                queue.push_back(ny * w + nx);
            }
        }
    }

    /// @brief Follows the solved path downhill from a pixel and returns the pixel reached after the lookahead
    [[nodiscard]] b2Vec2 next_waypoint_px(int x, int y) const
    {
        const int w = static_cast<int>(image.getSize().x);
        constexpr int DX[4]{1, -1, 0, 0};
        constexpr int DY[4]{0, 0, 1, -1};
        for (int step = 0; step < JUMP_LOOKAHEAD_PX; ++step)
        {
            int best = path_dist[static_cast<std::size_t>(y) * w + x];
            int bx = x;
            int by = y;
            for (int k = 0; k < 4; ++k)
            {
                const int nx = x + DX[k];
                const int ny = y + DY[k];
                if (is_wall(nx, ny))
                {
                    continue;
                }
                const int d = path_dist[static_cast<std::size_t>(ny) * w + nx];
                if (d >= 0 && d < best)
                {
                    best = d;
                    bx = nx;
                    by = ny;
                }
            }
            if (bx == x && by == y)
            {
                break;
            }
            x = bx;
            y = by;
        }
        return b2Vec2{static_cast<float>(x), static_cast<float>(y)};
    }

    /// @brief A ball that barely moved for a while is stuck, so it jumps toward the next step of the solved path
    void jump_stuck_balls()
    {
        for (auto &ball : balls)
        {
            ball.stuck_timer_seconds += FIXED_DELTA;
            if (ball.stuck_timer_seconds < STUCK_CHECK_SECONDS)
            {
                continue;
            }
            ball.stuck_timer_seconds = 0.0f;

            const b2Vec2 pos = b2Body_GetPosition(ball.body);
            const float travel = std::hypot(pos.x - ball.last_checked_pos.x, pos.y - ball.last_checked_pos.y);
            ball.last_checked_pos = pos;
            if (travel >= STUCK_MIN_TRAVEL_M)
            {
                continue;
            }

            const int px = static_cast<int>(pos.x * PIXELS_PER_METER);
            const int py = static_cast<int>(pos.y * PIXELS_PER_METER);
            if (is_wall(px, py) || path_dist[static_cast<std::size_t>(py) * image.getSize().x + px] <= 0)
            {
                continue;
            }
            const b2Vec2 target = next_waypoint_px(px, py);
            const float dx = target.x - static_cast<float>(px);
            const float dy = target.y - static_cast<float>(py);
            const float len = std::hypot(dx, dy);
            if (len < 1.0e-3f)
            {
                continue;
            }
            const float impulse = b2Body_GetMass(ball.body) * JUMP_SPEED_MPS;
            b2Body_ApplyLinearImpulseToCenter(ball.body, b2Vec2{dx / len * impulse, dy / len * impulse}, true);
        }
    }

    /// @brief Random position, in meters, whose whole ball footprint is on floor pixels
    [[nodiscard]] b2Vec2 pick_floor_position(const mazes::randomizer &rng) const
    {
        const auto size = image.getSize();
        const int margin = static_cast<int>(std::ceil(dynamic_ball::RADIUS_M * PIXELS_PER_METER)) + 1;
        constexpr int MAX_ATTEMPTS = 1000;
        const auto xs = rng.sample_int_n(margin, static_cast<int>(size.x) - 1 - margin, MAX_ATTEMPTS);
        const auto ys = rng.sample_int_n(margin, static_cast<int>(size.y) - 1 - margin, MAX_ATTEMPTS);
        for (std::size_t attempt = 0; attempt < xs.size() && attempt < ys.size(); ++attempt)
        {
            const int px = xs[attempt];
            const int py = ys[attempt];
            if (!is_wall(px - margin, py) && !is_wall(px + margin, py) && !is_wall(px, py - margin) &&
                !is_wall(px, py + margin) && !is_wall(px, py))
            {
                return b2Vec2{(static_cast<float>(px) + 0.5f) / PIXELS_PER_METER,
                              (static_cast<float>(py) + 0.5f) / PIXELS_PER_METER};
            }
        }
        return b2Vec2{world_w * 0.5f, world_h * 0.5f};
    }

    /// @brief Static boxes covering every wall pixel, so physics matches the drawn image exactly.
    ///        Horizontal runs are merged into one box, then identical runs on adjacent rows are stacked
    void build_walls_from_image()
    {
        const auto size = image.getSize();
        wall_color = image.getPixel({0u, 0u});

        b2BodyDef body_def = b2DefaultBodyDef();
        const b2BodyId walls = b2CreateBody(world, &body_def);
        const b2ShapeDef shape_def = b2DefaultShapeDef();

        struct run
        {
            int x0, x1, y0, y1;
        };
        std::vector<run> open_runs;
        const auto emit = [&](const run &r)
        {
            const float hx = static_cast<float>(r.x1 - r.x0) * 0.5f / PIXELS_PER_METER;
            const float hy = static_cast<float>(r.y1 - r.y0) * 0.5f / PIXELS_PER_METER;
            const b2Vec2 center{(static_cast<float>(r.x0 + r.x1) * 0.5f) / PIXELS_PER_METER,
                                (static_cast<float>(r.y0 + r.y1) * 0.5f) / PIXELS_PER_METER};
            const b2Polygon box = b2MakeOffsetBox(hx, hy, center, b2MakeRot(0.0f));
            b2CreatePolygonShape(walls, &shape_def, &box);
        };

        for (int y = 0; y < static_cast<int>(size.y); ++y)
        {
            std::vector<run> row_runs;
            for (int x = 0; x < static_cast<int>(size.x);)
            {
                if (!is_wall(x, y))
                {
                    ++x;
                    continue;
                }
                const int start = x;
                while (x < static_cast<int>(size.x) && is_wall(x, y))
                {
                    ++x;
                }
                row_runs.push_back({start, x, y, y + 1});
            }

            std::vector<run> next_open;
            for (auto &r : row_runs)
            {
                const auto match = std::ranges::find_if(open_runs, [&](const run &o)
                                                        { return o.x0 == r.x0 && o.x1 == r.x1; });
                if (match != open_runs.end())
                {
                    r.y0 = match->y0;
                    open_runs.erase(match);
                }
                next_open.push_back(r);
            }
            for (const auto &finished : open_runs)
            {
                emit(finished);
            }
            open_runs = std::move(next_open);
        }
        for (const auto &finished : open_runs)
        {
            emit(finished);
        }
    }

    sf::Image image;
    sf::Color wall_color;
    std::vector<int> path_dist;
    float world_w{1.0f};
    float world_h{1.0f};
    b2WorldId world{b2_nullWorldId};
    std::vector<dynamic_ball> balls;
};

/// @brief Runs apply() with fresh random args and loads the resulting image
bool generate_maze(const mazes::randomizer &rng, sf::Image &image)
{
    const auto runtime_inst = mazes::runtime_app::instance();
    if (runtime_inst == nullptr)
    {
        return false;
    }
    std::error_code ec;
    std::filesystem::remove(TEMP_IMAGE_PATH, ec);

    const auto start = std::chrono::steady_clock::now();
    (void)runtime_inst->apply(generate_random_args(rng));
    const std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - start;
    std::printf("Apply took %.3f ms\n", elapsed.count());

    return image.loadFromFile(TEMP_IMAGE_PATH);
}

/// @brief Fits the window and sprite to the image and returns the pixels-per-meter of the board
float fit_to_image(sf::RenderWindow &window, sf::Sprite &sprite, const sf::Image &image)
{
    const auto size = image.getSize();
    const float scale = std::max(1.0f, std::floor(static_cast<float>(MAX_WINDOW_DIM_PX) / static_cast<float>(std::max(size.x, size.y))));
    sprite.setScale({scale, scale});
    window.setSize({static_cast<unsigned int>(size.x * scale), static_cast<unsigned int>(size.y * scale)});
    window.setView(sf::View{sf::FloatRect{{0.0f, 0.0f}, {size.x * scale, size.y * scale}}});
    return PIXELS_PER_METER * scale;
}

int main(int, char *argv[])
{
    try
    {
        const mazes::randomizer rng;
        sf::Image image;
        if (!generate_maze(rng, image))
        {
            throw std::runtime_error("failed to generate a maze image at " + TEMP_IMAGE_PATH.string());
        }

        sf::Texture texture;
        if (!texture.loadFromImage(image))
        {
            throw std::runtime_error("failed to create a texture from the maze image");
        }
        sf::Sprite sprite{texture};
        auto physics = std::make_unique<ball_world>(image, rng);

        const std::filesystem::path resource_dir = std::filesystem::absolute(argv[0]).parent_path() / "resources";

        // Both shaders are optional, without them the maze is drawn plain
        const bool shaders_ok = sf::Shader::isAvailable();
        sf::Shader parallax_shader;
        sf::Shader cutout_shader;
        const bool parallax_loaded = shaders_ok && parallax_shader.loadFromFile(resource_dir / "parallax.frag", sf::Shader::Type::Fragment);
        const bool cutout_loaded = parallax_loaded && cutout_shader.loadFromMemory(std::string{WALL_CUTOUT_FRAG}, sf::Shader::Type::Fragment);
        if (!cutout_loaded)
        {
            std::fprintf(stderr, "Parallax shaders unavailable, drawing the maze without them\n");
        }
        const auto update_wall_color = [&]()
        {
            if (cutout_loaded)
            {
                const sf::Color wall = image.getPixel({0u, 0u});
                const sf::Glsl::Vec3 wall_vec{static_cast<float>(wall.r) / 255.0f, static_cast<float>(wall.g) / 255.0f, static_cast<float>(wall.b) / 255.0f};
                cutout_shader.setUniform("wall_color", wall_vec);
                parallax_shader.setUniform("wallColor", wall_vec);
            }
        };
        update_wall_color();

        sf::RenderWindow window{sf::VideoMode{{MAX_WINDOW_DIM_PX, MAX_WINDOW_DIM_PX}, 32}, APP_NAME};
        window.setFramerateLimit(120u);
        sf::Image windowIcon;
        if (windowIcon.loadFromFile(resource_dir / "icon.bmp"))
        {
            window.setIcon(windowIcon);
        }
        float pixels_per_meter = fit_to_image(window, sprite, image);

        sf::CircleShape shape{dynamic_ball::RADIUS_M * pixels_per_meter};
        shape.setFillColor(sf::Color::White);
        shape.setOutlineThickness(1.0f);
        shape.setOutlineColor(sf::Color::Black);

        std::printf("%s\n", "Press B to generate a new maze");

        sf::Clock clock;
        const sf::Clock total_clock;
        float accumulator{0.0f};
        while (window.isOpen())
        {
            while (const auto event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                {
                    window.close();
                }
                else if (const auto *key = event->getIf<sf::Event::KeyPressed>())
                {
                    if (key->code == sf::Keyboard::Key::Escape)
                    {
                        window.close();
                    }
                    else if (key->code == sf::Keyboard::Key::B && generate_maze(rng, image) && texture.loadFromImage(image))
                    {
                        sprite.setTexture(texture, true);
                        update_wall_color();
                        physics = std::make_unique<ball_world>(image, rng);
                        pixels_per_meter = fit_to_image(window, sprite, image);
                        shape.setRadius(dynamic_ball::RADIUS_M * pixels_per_meter);
                    }
                }
            }

            accumulator = std::min(accumulator + clock.restart().asSeconds(), 0.25f);
            while (accumulator >= FIXED_DELTA)
            {
                physics->step(1);
                accumulator -= FIXED_DELTA;
            }

            window.clear(sf::Color::Black);
            if (cutout_loaded)
            {
                const sf::Vector2f window_size{window.getView().getSize()};
                float mean_x{0.0f};
                for (const auto &ball : physics->get_balls())
                {
                    mean_x += b2Body_GetPosition(ball.body).x;
                }
                mean_x /= static_cast<float>(physics->get_balls().size());

                parallax_shader.setUniform("iResolution", sf::Glsl::Vec2{window_size});
                const sf::FloatRect bounds = sprite.getGlobalBounds();
                parallax_shader.setUniform("maze", texture);
                parallax_shader.setUniform("mazeRect", sf::Glsl::Vec4{bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y});
                parallax_shader.setUniform("cellUv", static_cast<float>(CELL_PITCH_PX) / static_cast<float>(texture.getSize().x));
                parallax_shader.setUniform("iTime", total_clock.getElapsedTime().asSeconds() / 10.0f);
                parallax_shader.setUniform("iOffset", (mean_x / physics->get_world_width() - 0.5f) * PARALLAX_OFFSET_SCALE);

                sf::RectangleShape background{window_size};
                window.draw(background, &parallax_shader);
                window.draw(sprite, &cutout_shader);
            }
            else
            {
                window.draw(sprite);
            }
            shape.setOrigin({shape.getRadius(), shape.getRadius()});
            for (const auto &ball : physics->get_balls())
            {
                const b2Vec2 pos = b2Body_GetPosition(ball.body);
                shape.setPosition({pos.x * pixels_per_meter, pos.y * pixels_per_meter});
                shape.setFillColor(ball.is_positive() ? sf::Color{80, 220, 90} : sf::Color{230, 70, 70});
                window.draw(shape);
            }
            window.display();
        }
    }
    catch (const std::exception &ex)
    {
        std::fprintf(stderr, "Unhandled exception: %s\n", ex.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
