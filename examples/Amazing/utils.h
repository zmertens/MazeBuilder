#ifndef UTILS_H
#define UTILS_H

#include <filesystem>
#include <functional>
#include <initializer_list>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/singleton_base.h>

namespace mazes
{
    class randomizer;
}

namespace sf
{
    class Font;
    class RenderWindow;
    class Shader;
    class Texture;
    class SoundBuffer;
    class Sound;
}

namespace utils
{
    class async_loader : public mazes::singleton_base<async_loader>
    {
        friend class mazes::singleton_base<async_loader>;

    public:
        static const std::filesystem::path &get_resource_path() noexcept;
        static void set_resource_path(const std::filesystem::path &path) noexcept;

        static std::map<std::string, std::filesystem::path> load_resource_map(
            const std::filesystem::path &resource_json = get_resource_path());

        static std::optional<std::filesystem::path> resolve_resource_path(
            std::string_view key,
            const std::filesystem::path &resource_json = get_resource_path());

        static std::optional<std::filesystem::path> resolve_resource_path(
            std::string_view key,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static std::optional<std::filesystem::path> resolve_first_existing_resource(
            std::initializer_list<std::string_view> keys,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static std::map<std::string, std::filesystem::path> load_required_resource_map(
            const std::filesystem::path &resource_json = get_resource_path());

        static bool try_set_window_icon(
            sf::RenderWindow &window,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            std::string_view icon_key,
            const std::filesystem::path &resource_json = get_resource_path());

        static bool try_load_font(
            sf::Font &font,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            std::string_view font_key,
            const std::filesystem::path &resource_json = get_resource_path());

        static bool ensure_fragment_shader_loaded(
            sf::Shader &shader,
            bool &is_loaded,
            std::initializer_list<std::string_view> fragment_keys,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static bool ensure_shader_loaded(
            sf::Shader &shader,
            bool &is_loaded,
            std::string_view vertex_key,
            std::string_view fragment_key,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static bool ensure_shader_loaded(
            sf::Shader &shader,
            bool &is_loaded,
            std::string_view vertex_key,
            std::string_view geometry_key,
            std::initializer_list<std::string_view> fragment_keys,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static bool ensure_texture_loaded(
            sf::Texture &texture,
            bool &is_loaded,
            std::initializer_list<std::string_view> texture_keys,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static bool ensure_sound_loaded(
            sf::SoundBuffer &buffer,
            sf::Sound &sound,
            bool &is_loaded,
            std::initializer_list<std::string_view> sound_keys,
            const std::map<std::string, std::filesystem::path> &loaded_resources,
            const std::filesystem::path &resource_json = get_resource_path());

        static void load(const std::function<void(std::map<std::string, std::filesystem::path> &)> &initializer) noexcept;

    private:
        static std::filesystem::path resource_path;
    };

    class physics_ops : public mazes::singleton_base<physics_ops>
    {
        friend class mazes::singleton_base<physics_ops>;

    public:
        static b2WorldId recreate_world(b2WorldId existing_world) noexcept;

        static b2Vec2 px_to_m(float x, float y, float pixels_per_meter) noexcept;
        static b2Vec2 screen_px_to_world_m(float x, float y, float pixels_per_meter, float world_scale_x, float world_scale_y) noexcept;
        static sf::Vector2f world_m_to_screen_px(const b2Vec2 &world_pos, float pixels_per_meter, float world_scale_x, float world_scale_y) noexcept;

        static std::optional<b2BodyId> add_wall_body_from_rect(
            b2WorldId world,
            float pixels_per_meter,
            float x,
            float y,
            float w,
            float h) noexcept;

        static int random_value_ball(mazes::randomizer &rng) noexcept;
        static sf::Color value_ball_tint(int value) noexcept;

        static void apply_linear_impulse_toward(
            b2BodyId from_body,
            b2BodyId to_body,
            float impulse_scale) noexcept;

        template <typename BallContainer>
        static std::optional<std::size_t> find_ball_at(
            const BallContainer &balls,
            const sf::Vector2f &pos_pixels,
            float pixels_per_meter,
            float world_scale_x,
            float world_scale_y,
            float pick_radius_px) noexcept
        {
            const b2Vec2 target = screen_px_to_world_m(
                pos_pixels.x,
                pos_pixels.y,
                pixels_per_meter,
                world_scale_x,
                world_scale_y);

            float best_dist_sq = 1e9f;
            std::optional<std::size_t> best_index;

            for (std::size_t i = 0; i < balls.size(); ++i)
            {
                if (balls[i].collected)
                {
                    continue;
                }

                const b2Vec2 p = b2Body_GetPosition(balls[i].body);
                const float dx = target.x - p.x;
                const float dy = target.y - p.y;
                const float d2 = dx * dx + dy * dy;
                const auto max_pick_radius_m = pick_radius_px / pixels_per_meter;
                const float max_pick_dist_sq = max_pick_radius_m * max_pick_radius_m;
                if (d2 <= max_pick_dist_sq && d2 < best_dist_sq)
                {
                    best_dist_sq = d2;
                    best_index = i;
                }
            }

            return best_index;
        }

        template <typename BallContainer>
        static void update_number_balls(BallContainer &balls, float dt, float fade_window_seconds = 5.0f) noexcept
        {
            for (auto it = balls.begin(); it != balls.end();)
            {
                auto &ball = *it;
                if (!ball.collected)
                {
                    ++it;
                    continue;
                }

                ball.lifetime_seconds -= dt;
                if (ball.lifetime_seconds <= 0.0f)
                {
                    if (B2_IS_NON_NULL(ball.body))
                    {
                        b2DestroyBody(ball.body);
                        ball.body = b2_nullBodyId;
                    }
                    it = balls.erase(it);
                    continue;
                }

                auto color = ball.drawable.getFillColor();
                const float fa = std::clamp(ball.lifetime_seconds / fade_window_seconds, 0.0f, 1.0f);
                color.a = static_cast<std::uint8_t>(std::clamp(255.0f * fa, 0.0f, 255.0f));
                ball.drawable.setFillColor(color);
                ++it;
            }
        }

        template <typename BallContainer>
        static void sync_ball_drawables(
            BallContainer &balls,
            float pixels_per_meter,
            float world_scale_x,
            float world_scale_y) noexcept
        {
            for (auto &ball : balls)
            {
                if (!B2_IS_NON_NULL(ball.body))
                {
                    continue;
                }

                const b2Vec2 p = b2Body_GetPosition(ball.body);
                ball.drawable.setPosition(world_m_to_screen_px(p, pixels_per_meter, world_scale_x, world_scale_y));
                ball.drawable.setScale({world_scale_x, world_scale_y});
            }
        }

        template <typename ParticleContainer, typename Randomizer>
        static void emit_bonus_particles(
            ParticleContainer &particles,
            Randomizer &rng,
            const sf::Vector2f &origin,
            std::size_t particle_count = 18u) noexcept
        {
            using particle_type = typename ParticleContainer::value_type;
            using drawable_type = std::remove_reference_t<decltype(std::declval<particle_type &>().drawable)>;

            for (std::size_t i = 0; i < particle_count; ++i)
            {
                const float angle = static_cast<float>(rng.get_int(0, 359)) * 3.14159265f / 180.0f;
                const float speed = static_cast<float>(rng.get_int(45, 155));
                particle_type p{
                    drawable_type{static_cast<float>(rng.get_int(2, 5))},
                    {std::cos(angle) * speed, std::sin(angle) * speed},
                    static_cast<float>(rng.get_int(35, 90)) / 100.0f};
                p.drawable.setOrigin({p.drawable.getRadius(), p.drawable.getRadius()});
                p.drawable.setPosition(origin);
                p.drawable.setFillColor(sf::Color(255, 219, 88, 210));
                p.drawable.setOutlineThickness(0.6f);
                p.drawable.setOutlineColor(sf::Color(255, 248, 196));
                particles.push_back(p);
            }
        }

        template <typename ParticleContainer>
        static void update_bonus_particles(ParticleContainer &particles, float dt) noexcept
        {
            for (auto &particle : particles)
            {
                particle.life_seconds -= dt;
                particle.drawable.move(particle.velocity * dt);
                particle.velocity *= 0.94f;
                const auto fill = particle.drawable.getFillColor();
                const auto alpha = static_cast<std::uint8_t>(std::clamp(particle.life_seconds, 0.0f, 1.0f) * 220.0f);
                particle.drawable.setFillColor(sf::Color(fill.r, fill.g, fill.b, alpha));
            }

            particles.erase(std::remove_if(particles.begin(), particles.end(), [](const auto &particle)
                                           { return particle.life_seconds <= 0.0f; }),
                            particles.end());
        }

        template <typename ParticleContainer, typename Target>
        static void draw_bonus_particles(const ParticleContainer &particles, Target &target)
        {
            for (const auto &particle : particles)
            {
                target.draw(particle.drawable);
            }
        }

        template <typename BallContainer, typename Target>
        static void draw_bonus_balls(const BallContainer &balls, Target &target)
        {
            for (const auto &ball : balls)
            {
                if (ball.collected && ball.lifetime_seconds <= 0.0f)
                {
                    continue;
                }

                target.draw(ball.drawable);
            }
        }

    private:
        physics_ops() = default;
    };
}

#endif // UTILS_H