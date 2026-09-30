#include "utils.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/randomizer.h>

#include <array>
#include <cmath>
#include <fstream>
#include <stdexcept>

using namespace utils;

namespace
{
    static mazes::async_logger LOGGER{};
}

std::filesystem::path async_loader::resource_path = {};

const std::filesystem::path &async_loader::get_resource_path() noexcept
{
    return resource_path;
}

void async_loader::set_resource_path(const std::filesystem::path &path) noexcept
{
    resource_path = path;
}

std::map<std::string, std::filesystem::path> async_loader::load_resource_map(
    const std::filesystem::path &resource_json)
{
    if (resource_json.empty() || !std::filesystem::exists(resource_json))
    {
        LOGGER.log("Resource map \'" + resource_json.string() + "\' was not found.");
        return {};
    }

    std::unordered_map<std::string, std::string> raw;
    if (!mazes::json_helper::load(resource_json.string(), raw))
    {
        LOGGER.log("Failed to parse resource map \'" + resource_json.string() + "\'");
        return {};
    }

    std::map<std::string, std::filesystem::path> resources;
    for (const auto &[key, value] : raw)
    {
        const auto resolved = (resource_json.parent_path() / value).lexically_normal();
        resources.emplace(key, resolved);
    }

    return resources;
}

std::optional<std::filesystem::path> async_loader::resolve_resource_path(
    std::string_view key,
    const std::filesystem::path &resource_json)
{
    const auto resources = load_resource_map(resource_json);
    const auto match = resources.find(std::string{key});
    if (match == resources.end())
    {
        return std::nullopt;
    }

    const auto &resolved = match->second;
    if (!resolved.empty() && std::filesystem::exists(resolved))
    {
        return resolved;
    }

    return std::nullopt;
}

std::optional<std::filesystem::path> async_loader::resolve_resource_path(
    std::string_view key,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    const auto it = loaded_resources.find(std::string{key});
    if (it != loaded_resources.cend())
    {
        return it->second;
    }

    return resolve_resource_path(key, resource_json);
}

std::optional<std::filesystem::path> async_loader::resolve_first_existing_resource(
    std::initializer_list<std::string_view> keys,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    for (const auto key : keys)
    {
        if (const auto resolved = resolve_resource_path(key, loaded_resources, resource_json);
            resolved.has_value() && std::filesystem::exists(*resolved))
        {
            return resolved;
        }
    }

    return std::nullopt;
}

std::map<std::string, std::filesystem::path> async_loader::load_required_resource_map(
    const std::filesystem::path &resource_json)
{
    const auto loader_inst = instance();
    if (!loader_inst)
    {
        throw std::runtime_error("Amazing async loader instance was null");
    }

    loader_inst->set_resource_path(resource_json);

    if (!std::filesystem::exists(resource_json) || resource_json.extension() != ".json")
    {
        throw std::runtime_error("Amazing failed to load resource_paths.json");
    }

    std::map<std::string, std::filesystem::path> resource_map;
    loader_inst->load([&resource_map](std::map<std::string, std::filesystem::path> &resources)
                      { resource_map = resources; });

    if (resource_map.empty())
    {
        resource_map = load_resource_map(resource_json);
    }

    if (resource_map.empty())
    {
        throw std::runtime_error("Amazing resource map loaded but was empty");
    }

    return resource_map;
}

std::map<std::string, std::string> async_loader::load_config_values(
    const std::filesystem::path &resource_json)
{
    if (resource_json.empty() || !std::filesystem::exists(resource_json))
    {
        LOGGER.log("Config map \'" + resource_json.string() + "\' was not found.");
        return {};
    }

    std::unordered_map<std::string, std::string> raw;
    if (!mazes::json_helper::load(resource_json.string(), raw))
    {
        LOGGER.log("Failed to parse config map \'" + resource_json.string() + "\'");
        return {};
    }

    return {raw.cbegin(), raw.cend()};
}

int async_loader::config_int(
    const std::map<std::string, std::string> &config_values,
    const std::string_view key,
    const int fallback) noexcept
{
    const auto it = config_values.find(std::string{key});
    if (it == config_values.cend())
    {
        return fallback;
    }

    try
    {
        return std::stoi(it->second);
    }
    catch (const std::exception &)
    {
        return fallback;
    }
}

bool async_loader::config_bool(
    const std::map<std::string, std::string> &config_values,
    const std::string_view key,
    const bool fallback) noexcept
{
    const auto it = config_values.find(std::string{key});
    if (it == config_values.cend())
    {
        return fallback;
    }

    const auto &value = it->second;
    if (value == "true" || value == "1")
    {
        return true;
    }
    if (value == "false" || value == "0")
    {
        return false;
    }

    return fallback;
}

bool async_loader::try_set_window_icon(
    sf::RenderWindow &window,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    std::string_view icon_key,
    const std::filesystem::path &resource_json)
{
    const auto icon_path = resolve_resource_path(icon_key, loaded_resources, resource_json);
    if (!icon_path.has_value())
    {
        return false;
    }

    sf::Image icon;
    if (!icon.loadFromFile(icon_path->string()))
    {
        return false;
    }

    window.setIcon(icon.getSize(), icon.getPixelsPtr());
    return true;
}

bool async_loader::try_load_font(
    sf::Font &font,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    std::string_view font_key,
    const std::filesystem::path &resource_json)
{
    if (const auto font_path = resolve_resource_path(font_key, loaded_resources, resource_json);
        font_path.has_value() && font.openFromFile(font_path->string()))
    {
        return true;
    }

    static constexpr std::array<std::string_view, 6> CANDIDATE_PATHS{
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf"};

    for (const auto candidate : CANDIDATE_PATHS)
    {
        const std::filesystem::path path{candidate};
        if (std::filesystem::exists(path) && font.openFromFile(path))
        {
            return true;
        }
    }

    return false;
}

bool async_loader::ensure_fragment_shader_loaded(
    sf::Shader &shader,
    bool &is_loaded,
    std::initializer_list<std::string_view> fragment_keys,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    if (is_loaded)
    {
        return true;
    }

    const auto fragment_path = resolve_first_existing_resource(fragment_keys, loaded_resources, resource_json);
    if (!fragment_path.has_value())
    {
        return false;
    }

    is_loaded = shader.loadFromFile(fragment_path->string(), sf::Shader::Type::Fragment);
    return is_loaded;
}

bool async_loader::ensure_shader_loaded(
    sf::Shader &shader,
    bool &is_loaded,
    std::string_view vertex_key,
    std::string_view fragment_key,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    if (is_loaded)
    {
        return true;
    }

    const auto vertex_path = resolve_resource_path(vertex_key, loaded_resources, resource_json);
    const auto fragment_path = resolve_resource_path(fragment_key, loaded_resources, resource_json);
    if (!vertex_path.has_value() || !fragment_path.has_value())
    {
        return false;
    }

    is_loaded = shader.loadFromFile(vertex_path->string(), fragment_path->string());
    return is_loaded;
}

bool async_loader::ensure_shader_loaded(
    sf::Shader &shader,
    bool &is_loaded,
    std::string_view vertex_key,
    std::string_view geometry_key,
    std::initializer_list<std::string_view> fragment_keys,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    if (is_loaded)
    {
        return true;
    }

    const auto vertex_path = resolve_resource_path(vertex_key, loaded_resources, resource_json);
    const auto geometry_path = resolve_resource_path(geometry_key, loaded_resources, resource_json);
    const auto fragment_path = resolve_first_existing_resource(fragment_keys, loaded_resources, resource_json);
    if (!vertex_path.has_value() || !geometry_path.has_value() || !fragment_path.has_value())
    {
        return false;
    }

    is_loaded = shader.loadFromFile(
        vertex_path->string(),
        geometry_path->string(),
        fragment_path->string());
    return is_loaded;
}

bool async_loader::ensure_texture_loaded(
    sf::Texture &texture,
    bool &is_loaded,
    std::initializer_list<std::string_view> texture_keys,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    if (is_loaded)
    {
        return true;
    }

    const auto texture_path = resolve_first_existing_resource(texture_keys, loaded_resources, resource_json);
    if (!texture_path.has_value())
    {
        return false;
    }

    is_loaded = texture.loadFromFile(texture_path->string());
    return is_loaded;
}

bool async_loader::ensure_sound_loaded(
    sf::SoundBuffer &buffer,
    sf::Sound &sound,
    bool &is_loaded,
    std::initializer_list<std::string_view> sound_keys,
    const std::map<std::string, std::filesystem::path> &loaded_resources,
    const std::filesystem::path &resource_json)
{
    if (is_loaded)
    {
        return true;
    }

    const auto sound_path = resolve_first_existing_resource(sound_keys, loaded_resources, resource_json);
    if (!sound_path.has_value())
    {
        return false;
    }

    if (!buffer.loadFromFile(sound_path->string()))
    {
        return false;
    }

    sound.setBuffer(buffer);
    is_loaded = true;
    return true;
}

void async_loader::load(const std::function<void(std::map<std::string, std::filesystem::path> &)> &initializer) noexcept
{
    try
    {
        if (resource_path.empty())
        {
            LOGGER.log("No resource path set for async loader.");
            return;
        }

        auto &&resource_map = load_resource_map(resource_path);
        LOGGER.log("Loaded " + std::to_string(resource_map.size()) + " resources from \'" + resource_path.string() + "\'");
        initializer(std::ref(resource_map));
    }
    catch (const std::exception &ex)
    {
        LOGGER.log("Exception caught during resource loading: " + std::string(ex.what()));
    }
}

b2WorldId physics_ops::recreate_world(const b2WorldId existing_world) noexcept
{
    if (B2_IS_NON_NULL(existing_world))
    {
        b2DestroyWorld(existing_world);
    }

    b2WorldDef def = b2DefaultWorldDef();
    def.gravity = {0.0f, 0.0f};
    return b2CreateWorld(&def);
}

b2Vec2 physics_ops::px_to_m(const float x, const float y, const float pixels_per_meter) noexcept
{
    return {x / pixels_per_meter, y / pixels_per_meter};
}

b2Vec2 physics_ops::screen_px_to_world_m(
    const float x,
    const float y,
    const float pixels_per_meter,
    const float world_scale_x,
    const float world_scale_y) noexcept
{
    return px_to_m(x / world_scale_x, y / world_scale_y, pixels_per_meter);
}

sf::Vector2f physics_ops::world_m_to_screen_px(
    const b2Vec2 &world_pos,
    const float pixels_per_meter,
    const float world_scale_x,
    const float world_scale_y) noexcept
{
    return {world_pos.x * pixels_per_meter * world_scale_x, world_pos.y * pixels_per_meter * world_scale_y};
}

std::optional<b2BodyId> physics_ops::add_wall_body_from_rect(
    const b2WorldId world,
    const float pixels_per_meter,
    const float x,
    const float y,
    const float w,
    const float h) noexcept
{
    if (!B2_IS_NON_NULL(world) || w <= 0.0f || h <= 0.0f)
    {
        return std::nullopt;
    }

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_staticBody;
    body_def.position = px_to_m(x + w * 0.5f, y + h * 0.5f, pixels_per_meter);
    const b2BodyId body = b2CreateBody(world, &body_def);

    b2ShapeDef shape_def = b2DefaultShapeDef();
    const b2Polygon box = b2MakeBox((w * 0.5f) / pixels_per_meter, (h * 0.5f) / pixels_per_meter);
    b2CreatePolygonShape(body, &shape_def, &box);

    return body;
}

int physics_ops::random_value_ball(mazes::randomizer &rng) noexcept
{
    const bool negative = rng.get_int(0, 9) == 0;
    const int magnitude = rng.get_int(1, 9);
    return negative ? -magnitude : magnitude;
}

sf::Color physics_ops::value_ball_tint(const int value) noexcept
{
    if (value > 0)
    {
        return sf::Color(255, 210, 92);
    }
    if (value < 0)
    {
        return sf::Color(255, 112, 120);
    }
    return sf::Color(220, 224, 255);
}

void physics_ops::apply_linear_impulse_toward(
    const b2BodyId from_body,
    const b2BodyId to_body,
    const float impulse_scale) noexcept
{
    if (!B2_IS_NON_NULL(from_body) || !B2_IS_NON_NULL(to_body))
    {
        return;
    }

    const b2Vec2 from_position = b2Body_GetPosition(from_body);
    const b2Vec2 to_position = b2Body_GetPosition(to_body);
    const float dx = to_position.x - from_position.x;
    const float dy = to_position.y - from_position.y;
    const float len_sq = dx * dx + dy * dy;
    if (len_sq <= 1e-6f)
    {
        return;
    }

    const float inv_len = 1.0f / std::sqrt(len_sq);
    const b2Vec2 impulse{dx * inv_len * impulse_scale, dy * inv_len * impulse_scale};
    b2Body_ApplyLinearImpulseToCenter(from_body, impulse, true);
}
