// Showcases basic voxel world and editing capabilities
// Follows game development patterns like Command Queues and State Stacks
// Creates mazes with Maze Builder library
// Originally written in C99, and then it got ported to C++20

#include "craft.h"

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include "fonts/Cousine_Regular.h"
#include "fonts/nunito_sans.h"
#include "fonts/Limelight_Regular.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <emscripten_local/emscripten_mainloop_stub.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include "db.h"
#include "geometries.h"
#include "resource_manager.h"
#include "font.h"
#include "player.h"
#include "resource_identifiers.h"
#include "sdl_gl_helper.h"
#include "shader.h"
#include "texture.h"
#include "world.h"

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ranges>

namespace
{
    static auto craft_using_gl_id{0u};
    static auto craft_preview_target_width{0};
    static auto craft_preview_target_height{0};

    enum class StackAction : unsigned int
    {
        PUSH = 0,
        POP = 1,
        CLEAR = 2
    };

    enum class StateIdentifier : unsigned int
    {
        DONE = 0,
        EDITOR = 1,
        LOADING = 2,
        MENU = 3,
        TOTAL = 4
    };
}

struct craft::craft_impl
{
    class state_stack;

    class state
    {
    public:
        virtual ~state() = default;
        typedef std::unique_ptr<state> ptr;

        struct context
        {
            explicit context(SDL_Window *window, font_manager &fonts, shader_manager &shaders, texture_manager &textures, player &p, sdl_gl_helper &sdl)
                : ctx_window{window}, ctx_fonts{&fonts}, ctx_shaders{&shaders}, ctx_textures{&textures}, active_player{&p},
                  ctx_sdl{&sdl}
            {
            }

            SDL_Window *ctx_window;
            font_manager *ctx_fonts;
            shader_manager *ctx_shaders;
            texture_manager *ctx_textures;
            player *active_player;
            sdl_gl_helper *ctx_sdl;
        };

        explicit state(state_stack &stack, const context &_context) : active_stack{&stack}, craft_ctx{_context}
        {
        }

        virtual void draw() const noexcept = 0;
        virtual bool update(float delta_time, mazes::randomizer &rng) noexcept = 0;
        virtual bool handle_event(SDL_Event &event) noexcept = 0;

    protected:
        void request_stack_push(const StateIdentifier state_id) const
        {
            active_stack->push_state(state_id);
        }

        void request_stack_pop() const
        {
            active_stack->pop_state();
        }

        void request_stack_clear() const
        {
            active_stack->clear_states();
        }

        [[nodiscard]] const context &get_context() const noexcept
        {
            return craft_ctx;
        }

        [[nodiscard]] state_stack &get_stack() const noexcept
        {
            return *active_stack;
        }

    private:
        state_stack *active_stack;
        context craft_ctx;
    };

    class state_stack
    {
        struct pending_change
        {
            explicit pending_change(StackAction action, StateIdentifier state_id = StateIdentifier::DONE)
                : action(action), state_id(state_id)
            {
            }

            StackAction action;
            StateIdentifier state_id;
        };

        std::vector<state::ptr> active_stack;
        std::vector<pending_change> pending_changes_list;
        state::context craft_ctx;
        std::map<StateIdentifier, std::function<state::ptr()>> change_factories;

    public:
        explicit state_stack(const state::context &_context)
            : craft_ctx(_context)
        {
        }

        template <typename T>
        void register_state(StateIdentifier state_id)
        {
            change_factories.insert_or_assign(state_id, [this]()
                                              { return state::ptr(std::make_unique<T>(*this, craft_ctx)); });
        }

        template <typename Pointer>
        [[nodiscard]] Pointer peek_state() const noexcept
        {
            auto reversed = active_stack | std::views::reverse;

            auto it = std::ranges::find_if(reversed, [](const auto &state_ptr)
                                           { return dynamic_cast<Pointer>(state_ptr.get()) != nullptr; });

            if (it != std::ranges::cend(reversed))
            {
                return dynamic_cast<Pointer>(it->get());
            }

            return nullptr;
        }

        // True when 's' is the topmost (active) state - update() now visits every state
        // unconditionally (background states keep ticking, e.g. so a Builder-tab preview
        // stays alive), so states must self-check this before doing "I'm in control" work
        // like grabbing mouse capture or processing realtime input.
        [[nodiscard]] bool is_top(const state *s) const noexcept
        {
            return !active_stack.empty() && active_stack.back().get() == s;
        }

        void update(const float delta_time, mazes::randomizer &rng) noexcept
        {
            apply_pending_changes();
            std::ranges::for_each(active_stack.rbegin(), active_stack.rend(), [&](const auto &state_ptr)
                                  { state_ptr->update(delta_time, std::ref(rng)); });
        }

        void draw() const noexcept
        {
            std::ranges::for_each(active_stack.rbegin(), active_stack.rend(), [](const auto &state_ptr)
                                  { state_ptr->draw(); });
        }

        void handle_event(SDL_Event &event) noexcept
        {
            for (auto it = active_stack.rbegin(); it != active_stack.rend(); ++it)
            {
                if (!(*it)->handle_event(event))
                {
                    break;
                }
            }

            apply_pending_changes();
        }

        void push_state(StateIdentifier state_id)
        {
            pending_changes_list.emplace_back(StackAction::PUSH, state_id);
        }

        void pop_state()
        {
            pending_changes_list.emplace_back(StackAction::POP);
        }

        void clear_states()
        {
            pending_changes_list.emplace_back(StackAction::CLEAR);
        }

        void apply_pending_changes()
        {
            for (const pending_change &change : pending_changes_list)
            {
                switch (change.action)
                {
                case StackAction::PUSH:
                    active_stack.push_back(create_state(change.state_id));
                    break;
                case StackAction::POP:
                    active_stack.pop_back();
                    break;
                case StackAction::CLEAR:
                    active_stack.clear();
                    break;
                }
            }
            pending_changes_list.clear();
        }

        [[nodiscard]] bool is_empty() const noexcept
        {
            return active_stack.empty();
        }

    private:
        state::ptr create_state(const StateIdentifier state_id)
        {
            if (const auto found = change_factories.find(state_id); found != change_factories.cend())
            {
                return found->second();
            }

            throw std::runtime_error("state_stack::create_state - No factory found for state ID");
        }
    }; // state_stack

    class loading_state final : public state
    {
        // Static once_flag to ensure load_resources is called only once across all instances
        std::atomic<bool> resources_loaded{false};
        static std::once_flag LOAD_RESOURCES_ONCE_FLAG;

        void load_resources() const noexcept
        {
            // fonts
            static constexpr float DEFAULT_FONT_PIXEL_SIZE = 18.f;

            auto &&ctx = get_context();
            font_manager *fonts = ctx.ctx_fonts;

            fonts->load(FontIdentifier::COUSINE_REGULAR, Cousine_Regular_compressed_data,
                        Cousine_Regular_compressed_size, DEFAULT_FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::LIMELIGHT, Limelight_Regular_compressed_data, Limelight_Regular_compressed_size,
                        DEFAULT_FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::NUNITO_SANS, NunitoSans_compressed_data, NunitoSans_compressed_size,
                        DEFAULT_FONT_PIXEL_SIZE);
            // shaders
            std::vector<std::tuple<ShaderIdentifier, std::string, std::string>> shader_programs{
                {ShaderIdentifier::BLOCK_SHADER,
                 "shaders/block_vertex.glsl",
                 "shaders/block_fragment.glsl"},
                {ShaderIdentifier::LINE_SHADER,
                 "shaders/line_vertex.glsl",
                 "shaders/line_fragment.glsl"},
                {ShaderIdentifier::SKY_SHADER,
                 "shaders/sky_vertex.glsl",
                 "shaders/sky_fragment.glsl"},
                {ShaderIdentifier::TEXT_SHADER,
                 "shaders/text_vertex.glsl",
                 "shaders/text_fragment.glsl"},
                {ShaderIdentifier::BLOOM_BLUR_SHADER,
                 "shaders/bloom_quad_vertex.glsl",
                 "shaders/bloom_blur_fragment.glsl"},
                {ShaderIdentifier::BLOOM_COMPOSITE_SHADER,
                 "shaders/bloom_quad_vertex.glsl",
                 "shaders/bloom_composite_fragment.glsl"}};

#if defined(__EMSCRIPTEN__)
            std::vector<std::tuple<ShaderIdentifier, std::string, std::string>> shader_gles_programs{
                {ShaderIdentifier::BLOCK_SHADER,
                 "shaders/es/block_vertex.es.glsl",
                 "shaders/es/block_fragment.es.glsl"},
                {ShaderIdentifier::LINE_SHADER,
                 "shaders/es/line_vertex.es.glsl",
                 "shaders/es/line_fragment.es.glsl"},
                {ShaderIdentifier::SKY_SHADER,
                 "shaders/es/sky_vertex.es.glsl",
                 "shaders/es/sky_fragment.es.glsl"},
                {ShaderIdentifier::TEXT_SHADER,
                 "shaders/es/text_vertex.es.glsl",
                 "shaders/es/text_fragment.es.glsl"},
                {ShaderIdentifier::BLOOM_BLUR_SHADER,
                 "shaders/es/bloom_quad_vertex.es.glsl",
                 "shaders/es/bloom_blur_fragment.es.glsl"},
                {ShaderIdentifier::BLOOM_COMPOSITE_SHADER,
                 "shaders/es/bloom_quad_vertex.es.glsl",
                 "shaders/es/bloom_composite_fragment.es.glsl"}};

            for (const auto &[id, vertex_path, fragment_path] : shader_gles_programs)
#else
            for (const auto &[id, vertex_path, fragment_path] : shader_programs)
#endif
            {
                ctx.ctx_shaders->load(id, vertex_path, fragment_path);

#if defined(MAZE_DEBUG)
                SDL_Log("Loaded shader: %d ( %s , %s )\n", static_cast<int>(id),
                        vertex_path.c_str(), fragment_path.c_str());
#endif
            }

            // textures
            constexpr std::string_view atlas_path = "textures/atlas.png";
            constexpr std::string_view bitmap_font_path = "textures/bitmap_font.png";
            constexpr std::string_view window_icon_path = "textures/icon.bmp";
            constexpr std::string_view signs_path = "textures/signs.png";
            constexpr std::string_view sky_path = "textures/sky.png";

            auto &&textures = ctx.ctx_textures;

            textures->load(TextureIdentifier::ATLAS, atlas_path, static_cast<unsigned int>(TextureIdentifier::ATLAS));
            textures->load(TextureIdentifier::BITMAP_FONT, bitmap_font_path,
                           static_cast<unsigned int>(TextureIdentifier::BITMAP_FONT));

            // Initialize MAZE texture with 1x1 white pixel placeholder
            constexpr std::uint8_t white_pixel[4] = {255, 255, 255, 255};
            textures->load(TextureIdentifier::MAZE, 1, 1, white_pixel,
                           static_cast<unsigned int>(TextureIdentifier::MAZE));

            textures->load(TextureIdentifier::SIGNS, signs_path, static_cast<unsigned int>(TextureIdentifier::SIGNS));
            textures->load(TextureIdentifier::SKY, sky_path, static_cast<unsigned int>(TextureIdentifier::SKY));
            textures->load(ctx.ctx_window, TextureIdentifier::WINDOW_ICON, window_icon_path);

#if defined(MAZE_DEBUG)

            std::ranges::for_each(LOADING_FONTS_WITH_NAMES, [](const auto &name)
                                  { SDL_Log("Loaded font: %s\n", name.data()); });

            SDL_Log("Loaded textures\n%s\n%s\n%s\n%s\n%s\n", atlas_path.data(),
                    bitmap_font_path.data(), window_icon_path.data(), signs_path.data(), sky_path.data());
#endif
        } // load_resources
    public:
        explicit loading_state(state_stack &_stack, const context &_context)
            : state(_stack, _context)
        {
        }

        void draw() const noexcept override
        {
            // Center the loading window
            const auto center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);

            // Style the loading window
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.067f, 0.137f, 0.094f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.118f, 0.227f, 0.161f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.933f, 1.0f, 0.8f, 1.0f));

            if (ImGui::Begin("Loading", nullptr,
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
            {
                ImGui::Spacing();
                ImGui::Spacing();

                // Center the text
                const auto loading_text = "Loading Resources...";
                const float text_width = ImGui::CalcTextSize(loading_text).x;
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_width) * 0.5f);
                ImGui::Text("%s", loading_text);

                ImGui::Spacing();
                ImGui::Spacing();

                // Show a simple progress bar or spinner effect
                ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(-1, 0), "");

                ImGui::Spacing();

                const char *status_text = resources_loaded ? "Complete!" : "Please wait...";
                const float status_width = ImGui::CalcTextSize(status_text).x;
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - status_width) * 0.5f);
                ImGui::TextColored(ImVec4(0.933f, 1.0f, 0.8f, 1.0f), "%s", status_text);
            }
            ImGui::End();

            ImGui::PopStyleColor(4);
        }

        bool update(const float delta_time, mazes::randomizer &rng) noexcept override
        {
            std::call_once(LOAD_RESOURCES_ONCE_FLAG, [this]()
                           { load_resources(); resources_loaded = true; });

            if (resources_loaded)
            {
                request_stack_pop();
            }

            return true;
        }

        bool handle_event(SDL_Event &event) noexcept override
        {
            return true;
        }

        bool resources_have_loaded() const noexcept
        {
            return resources_loaded;
        }
    }; // loading_state

    // Handles main gameplay workflow (building, editing, and rendering the voxel world)
    class editor_state final : public state
    {
        player &active_player;
        std::optional<world> current_voxel_world;

    public:
        explicit editor_state(state_stack &stack, const context &_context)
            : state{stack, _context}, active_player{*_context.active_player}
        {
        }

        void draw_preview3D(int fbo, int width, int height) const noexcept
        {
            if (current_voxel_world.has_value())
            {
                current_voxel_world->draw_preview(fbo, width, height);
            }
        }

        void draw() const noexcept override
        {
            if (current_voxel_world.has_value())
            {
                current_voxel_world->draw();
            }
        }

        void rebuild_world() noexcept
        {
            if (current_voxel_world.has_value())
            {
                current_voxel_world.reset();
            }
        }

        bool update(const float delta_time, mazes::randomizer &rng) noexcept override
        {
            auto &&ctx = get_context();
            auto &&stk = get_stack();
            const bool is_active_state = stk.is_top(this);

            // Initialize world only after loading state has finished
            if (!current_voxel_world.has_value())
            {
                if (auto *s = stk.peek_state<loading_state *>(); s && s->resources_have_loaded())
                {
                    current_voxel_world.emplace(ctx.ctx_window, *ctx.ctx_fonts,
                                                &active_player, *ctx.ctx_shaders, *ctx.ctx_textures,
                                                ctx.ctx_sdl);

                    current_voxel_world.value().init();

                    // Only grab the mouse when this state is actually in control.
                    if (is_active_state)
                    {
                        SDL_SetWindowRelativeMouseMode(get_context().ctx_window, true);
                    }

                    SDL_Log("Editor: World initialized after loading completed\n");
                }
            }

            if (current_voxel_world.has_value())
            {
                // Keep the world/daylight/chunk streaming alive in the background (so the
                // Builder-tab live preview stays current), but only touch mouse capture and
                // realtime input when this state is actually the one in control.
                current_voxel_world->update(delta_time, std::ref(rng));

                if (is_active_state)
                {
                    if (!SDL_GetWindowRelativeMouseMode(ctx.ctx_window))
                    {
                        SDL_SetWindowRelativeMouseMode(ctx.ctx_window, true);
                    }

                    active_player.update(delta_time, std::ref(rng));

                    auto &commands = current_voxel_world->get_command_queue();
                    active_player.handle_realtime_input(std::ref(commands));
                }
            }

            return true;
        }

        bool handle_event(SDL_Event &event) noexcept override
        {
            auto &&ctx = get_context();
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                SDL_Log("Editor: Received SDL_QUIT event - clearing stack\n");
                active_player.set_active(false);
                return false;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    SDL_SetWindowRelativeMouseMode(ctx.ctx_window, false);
                    request_stack_push(StateIdentifier::MENU);
                    return false;
                }
                break;

            default:
                break;
            }

            if (current_voxel_world.has_value())
            {
                auto &commands = current_voxel_world->get_command_queue();
                active_player.handle_event(event, std::ref(commands));
                current_voxel_world->handle_event(event);
            }

            return true;
        } // handle_event
    }; // editor_state

    // Handles GUI options
    class menu_state final : public state
    {
        std::list<std::string> algo_list;
        mutable std::string cached_artifacts;
        mutable bool artifact_export_in_progress{false};
        mutable bool rebuild_world_requested{false};

    public:
        explicit menu_state(state_stack &stack, const context &context)
            : state{stack, context}
        {
            algo_list.emplace_back(std::string{mazes::to_sv_from_algo(mazes::algo::BINARY_TREE)});
            algo_list.emplace_back(std::string{mazes::to_sv_from_algo(mazes::algo::DFS)});
            algo_list.emplace_back(std::string{mazes::to_sv_from_algo(mazes::algo::SIDEWINDER)});
        }

        void draw() const noexcept override
        {
            auto &&ctx = get_context();
            auto &&c = ctx.active_player->_configs;
            auto *p = ctx.active_player;
            // Default to last font in the list
            static auto selected_font_index{static_cast<int>(LOADING_FONTS_WITH_NAMES.size()) - 1};

            // Human-readable label for each PlayerAction (used by the key-bindings table).
            static constexpr auto action_label = [](const PlayerAction a) noexcept -> const char *
            {
                switch (a)
                {
                case PlayerAction::MOVE_AUTO:
                    return "Auto Run";
                case PlayerAction::MOVE_LEFT:
                    return "Move Left";
                case PlayerAction::MOVE_RIGHT:
                    return "Move Right";
                case PlayerAction::MOVE_FORWARD:
                    return "Move Forward";
                case PlayerAction::MOVE_BACKWARD:
                    return "Move Backward";
                case PlayerAction::MOVE_UP:
                    return "Fly Up";
                case PlayerAction::MOVE_DOWN:
                    return "Fly Down";
                case PlayerAction::JUMP:
                    return "Jump";
                case PlayerAction::FLY:
                    return "Toggle Fly";
                case PlayerAction::TAG_SIGN:
                    return "Tag Sign";
                case PlayerAction::BUILD_BLOCK:
                    return "Build Block";
                case PlayerAction::COPY_BLOCK:
                    return "Copy Block";
                case PlayerAction::DESTROY_BLOCK:
                    return "Destroy Block";
                case PlayerAction::PLACE_LIGHT:
                    return "Place Light";
                case PlayerAction::PLACE_MAZE:
                    return "Build Maze";
                case PlayerAction::PREVIEW_MAZE:
                    return "Preview Maze";
                case PlayerAction::CHANGE_PERSPECTIVE:
                    return "Change Perspective";
                case PlayerAction::ZOOM_IN_ISO_VIEW:
                    return "Zoom In Isometric View";
                case PlayerAction::ZOOM_OUT_ISO_VIEW:
                    return "Zoom Out Isometric View";
                default:
                    return "Unknown";
                }
            };

            // Forest-green theme – 19 colour pushes.
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.97f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.022f, 0.058f, 0.032f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.067f, 0.137f, 0.094f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.118f, 0.227f, 0.161f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.188f, 0.365f, 0.259f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.302f, 0.502f, 0.380f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.537f, 0.635f, 0.341f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.302f, 0.502f, 0.380f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.537f, 0.635f, 0.341f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.745f, 0.863f, 0.498f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.745f, 0.863f, 0.498f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.537f, 0.635f, 0.341f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.302f, 0.502f, 0.380f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.745f, 0.863f, 0.498f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabDimmed, ImVec4(0.188f, 0.365f, 0.259f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.302f, 0.502f, 0.380f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.537f, 0.635f, 0.341f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, ImVec4(0.188f, 0.365f, 0.259f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.933f, 1.0f, 0.8f, 1.0f));

            static constexpr ImVec4 HEADER_COL{0.745f, 0.863f, 0.498f, 1.0f};

            // Full-screen overlay window – no decorations, no resize, no move.
            const ImVec2 display = ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(display, ImGuiCond_Always);

            constexpr ImGuiWindowFlags win_flags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoSavedSettings;

            if (ImGui::Begin("##menu", nullptr, win_flags))
            {
                // ── Layout metrics (all scale automatically with FontGlobalScale) ──
                const ImGuiStyle &sty = ImGui::GetStyle();
                const float fs = ImGui::GetIO().FontGlobalScale;
                const float fhs = ImGui::GetFrameHeightWithSpacing();    // one widget row
                const float lhs = ImGui::GetTextLineHeightWithSpacing(); // one text row
                // Overhead per titled child-box: padding top+bottom, header, separator
                const float box_oh = sty.WindowPadding.y * 2.f + lhs + sty.ItemSpacing.y * 2.f + 1.f;
                const float sidebar_w = std::clamp(std::round(290.f * fs), 200.f, 420.f);
                const float btn_w = std::clamp(std::round(60.f * fs), 80.f, 170.f);

                // ── Title bar row ──────────────────────────────────────────────
                ImGui::TextColored(HEADER_COL, "  MazeBuilder Options");
                ImGui::SameLine(display.x - (btn_w * 2.f + sty.ItemSpacing.x + sty.WindowPadding.x));
                ImGui::Separator();

                const float avail_h = ImGui::GetContentRegionAvail().y;
                const float content_w = ImGui::GetContentRegionAvail().x - sidebar_w - sty.ItemSpacing.x;

                // LEFT SIDEBAR
                ImGui::BeginChild("##sidebar", ImVec2(sidebar_w, avail_h), ImGuiChildFlags_Borders);

                // Quick Options box
                bool last_show_maze_preview_2d_enabled{c.show_maze_preview_2d_enabled()};
                bool last_show_stats_window{c.show_stats_window()};
                ImGui::Checkbox("Preview Enabled", &last_show_maze_preview_2d_enabled);
                ImGui::Checkbox("Show Stats Overlay", &last_show_stats_window);
                ImGui::Separator();
                if (ImGui::Button("Enter World", ImVec2(btn_w * 2.f, 0.f)))
                {
                    c.show_maze_preview_2d_enabled(last_show_maze_preview_2d_enabled);
                    c.show_stats_window(last_show_stats_window);
                    request_stack_pop();
                }
                ImGui::Spacing();
                ImGui::Separator();

                // Key Bindings table
                ImGui::TextColored(HEADER_COL, "Key Bindings");
                ImGui::Separator();
                ImGui::Spacing();

                // Sidebar non-table content height (headers, checkboxes, mouse rows, close btn).
                const float sidebar_fixed = sty.WindowPadding.y * 2.f + 3.f * lhs + 2.f * fhs + 4.f * lhs + fhs + sty.ItemSpacing.y * 6.f + 8.f;
                const float keybind_h = std::max(3.f * fhs, avail_h - sidebar_fixed);
                if (ImGui::BeginTable("##keybinds", 2,
                                      ImGuiTableFlags_BordersOuter |
                                          ImGuiTableFlags_BordersInnerH |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_ScrollY,
                                      ImVec2(-1.f, keybind_h)))
                {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80.f);
                    ImGui::TableHeadersRow();

                    // Iterate enum in stable order; skip entries without a binding.
                    for (int i = 0; i < static_cast<int>(PlayerAction::COUNT); ++i)
                    {
                        const auto action = static_cast<PlayerAction>(i);
                        const auto scancode = static_cast<SDL_Scancode>(p->get_assigned_key(action));
                        if (scancode == SDL_SCANCODE_UNKNOWN)
                            continue;
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(action_label(action));
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(SDL_GetScancodeName(scancode));
                    }
                    ImGui::EndTable();
                }

                c.show_maze_preview_2d_enabled(last_show_maze_preview_2d_enabled)
                    .show_stats_window(last_show_stats_window);

                ImGui::Spacing();

                // ── Mouse Actions box ──────────────────────────────────────────
                ImGui::TextColored(HEADER_COL, "Mouse Actions");
                ImGui::Separator();
                if (ImGui::BeginTable("##mouse", 2, ImGuiTableFlags_None))
                {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
                    const char *mouse_rows[][2] = {
                        {"Left Click", "Destroy block"},
                        {"Right Click", "Build block"},
                        {"Middle Click", "Copy block type"},
                        {"Scroll", "Cycle block types"},
                    };
                    for (const auto &row : mouse_rows)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(row[0]);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(row[1]);
                    }
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::Separator();
                if (ImGui::Button("Close Application", ImVec2(-1.f, fhs)))
                    request_stack_clear();

                ImGui::EndChild(); // ##sidebar

                ImGui::SameLine();

                // ══ RIGHT CONTENT AREA ════════════════════════════════════════
                ImGui::BeginChild("##content", ImVec2(content_w, avail_h), ImGuiChildFlags_Borders);

                if (ImGui::BeginTabBar("##tabs"))
                {
                    // ── Builder tab ───────────────────────────────────────────
                    if (ImGui::BeginTabItem("Builder"))
                    {
                        ImGui::Separator();

                        auto *editor = get_stack().peek_state<editor_state *>();

                        int win_w = 0, win_h = 0;
                        SDL_GetWindowSizeInPixels(ctx.ctx_window, &win_w, &win_h);
                        auto target_h = win_h / 2;

                        ImGui::BeginChild("##builderpreview", ImVec2(0.f, target_h));

                        static std::uint32_t live_fbo = 0;
                        static std::uint32_t live_color_tex = 0;
                        static std::uint32_t live_depth_rbo = 0;
                        static int live_w = 0;
                        static int live_h = 0;

                        auto target_w = (win_h > 0)
                                            ? std::max(1, static_cast<int>(target_h * (static_cast<float>(win_w) / static_cast<float>(win_h))))
                                            : target_h;

                        if (live_fbo != 0)
                        {
                            glDeleteFramebuffers(1, &live_fbo);
                            glDeleteTextures(1, &live_color_tex);
                            glDeleteRenderbuffers(1, &live_depth_rbo);
                        }

                        glGenTextures(1, &live_color_tex);
                        glBindTexture(GL_TEXTURE_2D, live_color_tex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, target_w, target_h, 0,
                                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        glGenRenderbuffers(1, &live_depth_rbo);
                        glBindRenderbuffer(GL_RENDERBUFFER, live_depth_rbo);
                        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target_w, target_h);
                        glBindRenderbuffer(GL_RENDERBUFFER, 0);

                        glGenFramebuffers(1, &live_fbo);
                        glBindFramebuffer(GL_FRAMEBUFFER, live_fbo);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, live_color_tex, 0);
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, live_depth_rbo);
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);

                        live_w = target_w;
                        live_h = target_h;

                        editor->draw_preview3D(live_fbo, live_w, live_h);

                        const float aspect = static_cast<float>(live_w) / static_cast<float>(live_h);
                        float disp_w = ImGui::GetContentRegionAvail().x;
                        float disp_h = disp_w / aspect;
                        if (const auto avail_h = ImGui::GetContentRegionAvail().y; disp_h > avail_h)
                        {
                            disp_h = avail_h;
                            disp_w = disp_h * aspect;
                        }
                        // Flip V: FBO textures are bottom-up relative to ImGui's top-down UVs.
                        ImGui::Image(static_cast<ImTextureID>(live_color_tex), ImVec2(disp_w, disp_h),
                                     ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
                        ImGui::EndChild();

                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::Spacing();

                        if (ImGui::Button("Make New World", ImVec2(btn_w * 2.f, 0.f)))
                        {
                            rebuild_world_requested = true;
                        }

                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::Separator();

                        // ── Sign Message box ──────────────────────────────────
                        ImGui::BeginChild("##tag", ImVec2(0.f, box_oh + fhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Sign Message");
                        static char tag_buffer[256] = "";
                        static bool tag_init = false;
                        std::string current_tag = c.tag();
                        if (!tag_init || SDL_strcmp(tag_buffer, current_tag.data()) != 0)
                        {
                            SDL_strlcpy(tag_buffer, current_tag.data(), SDL_arraysize(tag_buffer));
                            tag_buffer[SDL_arraysize(tag_buffer) - 1] = '\0';
                            tag_init = true;
                        }
                        if (ImGui::InputText("##PlayerTag", tag_buffer, std::size(tag_buffer)))
                        {
                            c.tag(std::string{tag_buffer});
                        }
                        ImGui::EndChild();

                        ImGui::Spacing();

                        // ── Terrain Settings box ──────────────────────────────
                        ImGui::BeginChild("##terrain", ImVec2(0.f, box_oh + 2.f * lhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Terrain Settings");
                        ImGui::Separator();

                        auto has_heightmap_changed{c.show_heightmap()};
                        ImGui::Checkbox("Build with Mountains (requires new world)", &has_heightmap_changed);
                        if (has_heightmap_changed != c.show_heightmap())
                        {
                            c.show_heightmap(has_heightmap_changed);
                        }

                        ImGui::EndChild();

                        ImGui::Separator();
                        ImGui::Spacing();

                        auto &&maze_config = c.maze();
                        static std::string selected_algo;
                        selected_algo = std::string{mazes::to_sv_from_algo(maze_config.algo_id())};
                        static int rows = static_cast<int>(maze_config.rows());
                        static int columns = static_cast<int>(maze_config.columns());
                        static int levels = static_cast<int>(maze_config.levels());
                        static int seed = static_cast<int>(maze_config.seed());

                        // ── Maze Dimensions box ───────────────────────────────
                        ImGui::BeginChild("##dims", ImVec2(0.f, 160.f), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Maze Dimensions");
                        ImGui::Separator();
                        ImGui::SliderInt("Rows", &rows, 2, 10);
                        ImGui::SliderInt("Columns", &columns, 2, 10);
                        ImGui::SliderInt("Levels", &levels, 1, 5);
                        ImGui::EndChild();

                        ImGui::Spacing();

                        // ── Algorithm + Seed box ──────────────────────────────
                        ImGui::BeginChild("##algoseed", ImVec2(0.f, 110.f), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Algorithm & Seed");
                        ImGui::Separator();
                        if (constexpr ImGuiComboFlags combo_flags =
                                ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_WidthFitPreview;
                            ImGui::BeginCombo("Algorithm", selected_algo.data(), combo_flags))
                        {
                            for (const auto &itr : algo_list)
                            {
                                const bool is_selected = (mazes::to_algo_from_sv(itr) == maze_config.algo_id());
                                if (ImGui::Selectable(std::string{itr}.c_str(), is_selected))
                                {
                                    maze_config.algo_id(mazes::to_algo_from_sv(itr));
                                    selected_algo = itr;
                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SliderInt("Seed", &seed, 0, 1'000'000);
                        ImGui::EndChild();

                        ImGui::Separator();

                        // ── Instructions box ──────────────────────────────────
                        ImGui::BeginChild("##instruct", ImVec2(0.f, box_oh + 4.f * lhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Instructions");
                        ImGui::Separator();
                        ImGui::BulletText("Configure dimensions, algorithm and seed above");
                        ImGui::BulletText("Press 'Apply Configs'");
                        ImGui::BulletText("Press [E] in the editor to generate a preview");
                        ImGui::BulletText("Aim at a block face and press B to build");
                        ImGui::EndChild();

                        ImGui::Spacing();
                        if (ImGui::Button("Apply Configs", ImVec2(btn_w, btn_w * 0.5f)))
                        {
                            c.maze(mazes::configurator{}
                                       .algo_id(mazes::to_algo_from_sv(selected_algo))
                                       .rows(static_cast<unsigned int>(rows))
                                       .columns(static_cast<unsigned int>(columns))
                                       .levels(static_cast<unsigned int>(levels))
                                       .seed(static_cast<unsigned int>(seed)));
                        }

                        ImGui::EndTabItem();
                    }

                    // Graphics / Rendering tab
                    if (ImGui::BeginTabItem("Graphics"))
                    {
                        // Display box
                        ImGui::BeginChild("##display", ImVec2(0.f, 190.f), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Display");
                        ImGui::Separator();

                        bool last_vsync{c.vsync()};
                        bool last_fullscreen{c.fullscreen()};
                        bool last_invert_mouse{c.invert_mouse()};
                        float fov{c.fov()};
                        int ortho{c.ortho_scaling()};
                        int day{c.day_length()};
                        ImGui::Checkbox("Enable VSync", &last_vsync);
                        ImGui::Checkbox("Enable Fullscreen", &last_fullscreen);
                        ImGui::Checkbox("Invert Mouse Y-Axis", &last_invert_mouse);
                        ImGui::SliderFloat("Field of View", &fov, 30.f, 120.f, "%.1f deg");
                        ImGui::SliderInt("Orthographic Scale", &ortho, 0, 64);
                        ImGui::SliderInt("Day Length (s)", &day, 60, 1800);
                        ImGui::EndChild();

                        if (last_vsync != c.vsync())
                        {
                            SDL_GL_SetSwapInterval(last_vsync ? 1 : 0);
                        }
                        if (last_fullscreen != c.fullscreen())
                        {
                            SDL_SetWindowFullscreen(ctx.ctx_window, last_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
                        }

                        c.fov(std::clamp(fov, 30.f, 120.f))
                            .ortho_scaling(ortho)
                            .day_length(day)
                            .invert_mouse(last_invert_mouse)
                            .vsync(last_vsync)
                            .fullscreen(last_fullscreen);

                        ImGui::Spacing();

                        // Post-Process box
                        bool last_use_bloom{c.use_bloom_effect()};
                        float last_exposure{c.exposure_range()};
                        ImGui::BeginChild("##postfx", ImVec2(0.f, box_oh + 2.f * fhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Bloom / Post-Process");
                        ImGui::Separator();
                        ImGui::Checkbox("Enable Bloom Effect", &last_use_bloom);
                        ImGui::BeginDisabled(!last_use_bloom);
                        ImGui::SliderFloat("Exposure", &last_exposure, 0.1f, 2.0f, "%.2f");
                        ImGui::EndDisabled();
                        ImGui::EndChild();
                        c.use_bloom_effect(last_use_bloom).exposure_range(std::clamp(last_exposure, 0.1f, 2.0f));

                        ImGui::Spacing();

                        // ── Theme & Font box ──────────────────────────────────
                        {
                            const float listbox_h = 3.f * fhs + sty.FramePadding.y * 2.f;
                            const float theme_h = box_oh + 3.f * fhs + 2.f * sty.ItemSpacing.y + listbox_h;
                            ImGui::BeginChild("##theme", ImVec2(0.f, theme_h), ImGuiChildFlags_Borders);
                            ImGui::TextColored(HEADER_COL, "Theme & Font");
                            ImGui::Separator();
                            static bool dark_mode = true;
                            if (ImGui::Checkbox("Dark Mode", &dark_mode))
                            {
                                if (dark_mode)
                                    ImGui::StyleColorsDark();
                                else
                                    ImGui::StyleColorsLight();
                            }
                            ImGui::Spacing();

                            float last_font_scale{c.gui_font_scale()};
                            ImGui::SliderFloat("Font Scale", &last_font_scale, 0.6f, 1.4f, "%.2f");
                            ImGui::Spacing();
                            if (ImGui::BeginListBox("##FontList", ImVec2(-1.f, listbox_h)))
                            {
                                for (std::size_t i = 0; i < craft::craft_impl::SELECTABLE_FONTS.size(); ++i)
                                {
                                    const bool is_sel = (p->get_font_index() == static_cast<int>(i));
                                    const auto &font_name = craft_impl::LOADING_FONTS_WITH_NAMES.at(
                                        static_cast<std::size_t>(craft::craft_impl::SELECTABLE_FONTS.at(i)));
                                    if (ImGui::Selectable(font_name.data(), is_sel))
                                        p->set_font_index(static_cast<int>(i));
                                    if (is_sel)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndListBox();
                            }
                            ImGui::EndChild();

                            c.gui_font_scale(std::clamp(last_font_scale, 0.6f, 1.4f));
                        } // theme block scope

                        ImGui::EndTabItem();
                    }

                    // ── Player tab ────────────────────────────────────────────
                    if (ImGui::BeginTabItem("Player"))
                    {
                        // ── Identity box ──────────────────────────────────────
                        ImGui::BeginChild("##ident", ImVec2(0.f, box_oh + 2.f * lhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Identity");
                        ImGui::Separator();
                        ImGui::Text("Name: %s", p->get_name().c_str());
                        ImGui::Text("Local time: %s", p->get_local_time().data());
                        ImGui::EndChild();

                        ImGui::Spacing();

                        // ── Position box ──────────────────────────────────────
                        ImGui::BeginChild("##pos", ImVec2(0.f, box_oh + 2.f * lhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Position");
                        ImGui::Separator();
                        ImGui::Text("X: %.2f   Y: %.2f   Z: %.2f",
                                    p->pos.x, p->pos.y, p->pos.z);
                        ImGui::Text("Yaw: %.1f deg   Pitch: %.1f deg",
                                    p->pos.rx * (180.f / 3.14159f),
                                    p->pos.ry * (180.f / 3.14159f));
                        ImGui::EndChild();

                        ImGui::Spacing();

                        // ── Inventory box ─────────────────────────────────────
                        ImGui::BeginChild("##inv", ImVec2(0.f, box_oh + lhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Inventory");
                        ImGui::Separator();
                        ImGui::Text("Selected block type: %d", p->get_item());
                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }

                    // ── Exports tab ───────────────────────────────────────────
                    if (ImGui::BeginTabItem("Exports"))
                    {
                        ImGui::BeginChild("##export_box", ImVec2(0.f, box_oh + lhs + sty.ItemSpacing.y + 2.f * fhs), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Artifact Export");
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextWrapped("Export the current voxel world as a Wavefront OBJ file.");
                        ImGui::Spacing();

                        // Check export status and cache result when ready
                        if (artifact_export_in_progress && p->is_artifact_export_ready())
                        {
                            cached_artifacts = p->get_artifact_export_result();
                            artifact_export_in_progress = false;
                            SDL_Log("Async export complete - ready for download\n");
                        }

                        const bool export_in_progress = artifact_export_in_progress;

                        // Show status
                        if (export_in_progress)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f)); // Blue info
                            ImGui::TextWrapped("Export in progress... (check console for updates)");
                            ImGui::PopStyleColor();
                            // Simple spinner animation
                            const char *spinner_chars = "|/-\\";
                            const int spinner_idx = static_cast<int>(ImGui::GetTime() * 8) % 4;
                            ImGui::SameLine();
                            ImGui::Text("%c", spinner_chars[spinner_idx]);
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f)); // Green ready
                            ImGui::TextWrapped("Async export runs in background - UI stays responsive!");
                            ImGui::PopStyleColor();
                        }

                        ImGui::Spacing();

                        // Generate button (async or sync depending on size)
                        if (export_in_progress)
                        {
                            ImGui::BeginDisabled();
                        }

                        if (ImGui::Button("Generate Artifacts (Async)", ImVec2(0.f, 2.f * fhs)))
                        {
                            p->start_async_artifact_export();
                            cached_artifacts.clear(); // Clear old cache
                            artifact_export_in_progress = true;
                        }

                        if (export_in_progress)
                        {
                            ImGui::EndDisabled();
                        }

                        ImGui::SameLine();

                        // Download button (only enabled when artifacts are ready)
                        const bool has_artifacts = !cached_artifacts.empty() && !export_in_progress;
                        if (!has_artifacts)
                        {
                            ImGui::BeginDisabled();
                        }

                        if (ImGui::Button("Download File", ImVec2(0.f, 2.f * fhs)))
                        {
#if !defined(__EMSCRIPTEN__)
                            handle_artifacts(p, cached_artifacts);
#else
                            c.artifacts_ready(true);
                            handle_artifacts(p, cached_artifacts);
#endif
                        }

                        if (!has_artifacts)
                        {
                            ImGui::EndDisabled();
                        }

                        ImGui::EndChild();

                        ImGui::Spacing();
                        ImGui::BeginChild("##artifact_preview", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Preview (first 512 chars)");
                        ImGui::Separator();

                        // Display cached preview
                        if (cached_artifacts.empty())
                        {
                            ImGui::TextWrapped("No artifacts generated yet. Click 'Generate Artifacts' button above.");
                        }
                        else
                        {
                            const auto preview = cached_artifacts.substr(0, 512);
                            ImGui::TextWrapped("%s", preview.c_str());
                            if (cached_artifacts.size() > 512)
                            {
                                ImGui::TextDisabled("... (%zu more bytes)", cached_artifacts.size() - 512);
                            }
                        }

                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }

                    // ── CAD Tools tab ──────────────────────────────────────
                    if (ImGui::BeginTabItem("CAD Tools"))
                    {
                        // ── View Settings box ─────────────────────────────────
                        ImGui::BeginChild("##view_settings", ImVec2(0.f, 180.f), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "View Settings");
                        ImGui::Separator();

                        bool last_crosshair_details{c.show_crosshair_details()};
                        bool last_show_grid{c.show_grid_overlay()};
                        bool last_show_maze_preview_ghost{c.show_maze_preview_ghost()};
                        float last_grid_opacity{c.grid_opacity()};
                        int last_grid_spacing{c.grid_spacing()};

                        ImGui::Checkbox("Show Hover Info [H]", &last_crosshair_details);
                        ImGui::Spacing();
                        ImGui::Checkbox("Show Grid (G)", &last_show_grid);
                        if (last_show_grid)
                        {
                            ImGui::SliderInt("Grid Spacing", &last_grid_spacing, 1, 16);
                            ImGui::SliderFloat("Grid Opacity", &last_grid_opacity, 0.0f, 1.0f, "%.2f");
                        }
                        ImGui::Spacing();

                        ImGui::Checkbox("Show Maze Preview Ghost", &last_show_maze_preview_ghost);
                        ImGui::SameLine();

                        ImGui::EndChild();
                        ImGui::Spacing();

                        // ── Hotkeys box ───────────────────────────────────────
                        ImGui::BeginChild("##cad_hotkeys", ImVec2(0.f, 120.f), ImGuiChildFlags_Borders);
                        ImGui::TextColored(HEADER_COL, "Keyboard Shortcuts");
                        ImGui::Separator();
                        ImGui::BulletText("[G] - Toggle grid overlay");
                        ImGui::BulletText("[H] - Toggle crosshair details");
                        ImGui::BulletText("[M] - Toggle maze preview ghost");
                        ImGui::EndChild();

                        ImGui::EndTabItem();

                        c.show_crosshair_details(last_crosshair_details)
                            .show_grid_overlay(last_show_grid)
                            .grid_opacity(last_grid_opacity)
                            .grid_spacing(last_grid_spacing)
                            .show_maze_preview_ghost(last_show_maze_preview_ghost);
                    }

                    ImGui::EndTabBar();
                } // BeginTabBar

                ImGui::EndChild(); // ##content
            }
            ImGui::End(); // ##menu

            ImGui::PopStyleColor(19); // 19 colours pushed above
        }

        bool update(float delta_time, mazes::randomizer &rng) noexcept override
        {
            if (rebuild_world_requested)
            {
                rebuild_world_requested = false;

                return true;
            }
            return false;
        }

        bool handle_event(SDL_Event &event) noexcept override
        {
            if (SDL_EVENT_QUIT == event.type)
            {
                SDL_Log("Menu: Received SDL_QUIT event - clearing states\n");
                get_context().active_player->set_active(false);
                request_stack_clear();
            }

            // Handle ESCAPE to return to game
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                SDL_Log("Menu: ESCAPE pressed - returning to editor\n");
                request_stack_pop();
            }

            // Let window resize / fullscreen events propagate to the state below
            // (editor_state / runner_state) so the world can rebuild its FBOs.
            if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                return true;

            return false;
        }

        bool is_ready_to_rebuild_world() const noexcept
        {
            return rebuild_world_requested;
        }
    }; // menu_state

    const std::string &INIT_WINDOW_TITLE;
    const int INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT;

    static std::vector<std::string_view> LOADING_FONTS_WITH_NAMES;
    static std::vector<FontIdentifier> SELECTABLE_FONTS;

    std::unique_ptr<state_stack> crafting_states;

    font_manager active_fonts;
    shader_manager world_shaders;
    texture_manager world_textures;

    player active_player;

    sdl_gl_helper simple_direct_medialayer;

    mutable double fps_timer{0.0};
    mutable int smoothed_fps_counter{0};
    mutable float fps_timer_smoothed{0.0f};

    craft_impl(const std::string &title, const int w, const int h)
        : INIT_WINDOW_TITLE(title), INIT_WINDOW_WIDTH(w), INIT_WINDOW_HEIGHT(h)
    {
        start_SDL();

        setup_imgui();

        crafting_states = std::make_unique<state_stack>(state::context{
            simple_direct_medialayer.window,
            std::ref(active_fonts),
            std::ref(world_shaders),
            std::ref(world_textures),
            std::ref(active_player),
            std::ref(simple_direct_medialayer)});

        register_states();

        crafting_states->push_state(StateIdentifier::EDITOR);
        crafting_states->push_state(StateIdentifier::MENU);
        crafting_states->push_state(StateIdentifier::LOADING);

        SELECTABLE_FONTS.reserve(static_cast<std::size_t>(FontIdentifier::TOTAL));
        std::ranges::for_each(
            std::views::iota(0, static_cast<int>(FontIdentifier::TOTAL)),
            [this](const int id)
            {
                SELECTABLE_FONTS.push_back(static_cast<FontIdentifier>(id));
                return true;
            });
    }

    void register_states() const noexcept
    {
        crafting_states->register_state<editor_state>(StateIdentifier::EDITOR);
        crafting_states->register_state<loading_state>(StateIdentifier::LOADING);
        crafting_states->register_state<menu_state>(StateIdentifier::MENU);
    }

    void start_SDL() noexcept
    {
        if (simple_direct_medialayer.initialize(INIT_WINDOW_TITLE, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT))
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL initialized successfully.\n");
        }
    }

    void setup_imgui() const noexcept
    {
        // DEAR IMGUI INIT - Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
        ImGui::GetIO().IniFilename = nullptr;

        ImGui_ImplSDL3_InitForOpenGL(simple_direct_medialayer.window, simple_direct_medialayer.gl_context);

        std::string glsl_version;

#if defined(__EMSCRIPTEN__)
        glsl_version = "#version 100";
#else
        glsl_version = "#version 130";
#endif

        ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    }

    static std::string make_filename(const player *p) noexcept
    {
        auto &&temp = p->_configs.maze();
        const auto rows = std::to_string(temp.rows());
        const auto columns = std::to_string(temp.columns());
        const auto levels = std::to_string(temp.levels());

        std::string filename;
        filename.reserve(128);
        filename = rows + "x" + columns + "x" + levels + "_" + std::to_string(SDL_GetTicks()) + "_" + p->get_name() + ".obj";

        return filename;
    }

    static void handle_artifacts(player *p, const std::string &artifacts) noexcept
    {
        if (artifacts.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Failed to generate artifacts.\n");
            return;
        }

        const auto filename = make_filename(p);

#if !defined(__EMSCRIPTEN__)
        // Desktop: Write to file system and reset flag
        constexpr mazes::io_utils io_things{};
        const auto success = io_things.write_file(filename, artifacts);
        p->_configs.artifacts_ready(false);
        SDL_Log("Write file '%s': %s (%zu bytes)\n",
                filename.c_str(),
                success ? "SUCCESS" : "FAILED",
                artifacts.size());
#else
        // Web/Emscripten: Leave flag true so JavaScript can query artifacts
        // JavaScript should call craft.artifacts() to get the OBJ data
        SDL_Log("Web build: Artifacts ready for download via JavaScript\n");
#endif
    }

    void render_FPS() const noexcept
    {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always,
                                ImVec2(1.0f, 0.0f));

        ImGui::SetNextWindowBgAlpha(0.65f);

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                                 ImGuiWindowFlags_AlwaysAutoResize |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoFocusOnAppearing |
                                                 ImGuiWindowFlags_NoNav |
                                                 ImGuiWindowFlags_NoMove;

        if (this->active_player._configs.show_stats_window() && ImGui::Begin("FPS Overlay", nullptr, windowFlags))
        {
            ImGui::Text("FPS: %d", smoothed_fps_counter);
            ImGui::Text("ms / frame: %.2f ms", fps_timer_smoothed);
            ImGui::Text("Local time: %s\n", this->active_player.get_local_time().data());
            ImGui::End();
        }
    }

    void process_input() const noexcept
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            // Let ImGui process the event first
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                crafting_states->handle_event(event);
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                crafting_states->handle_event(event);
                continue;
            }

            // Window pixel-size events (resize / fullscreen) must always reach the
            // world so it can rebuild its FBOs — bypass the ImGui capture filter.
            if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
            {
                crafting_states->handle_event(event);
                continue;
            }

            // Check if ImGui wants to capture this event
            const ImGuiIO &io = ImGui::GetIO();
            const bool imgui_wants_keyboard = io.WantCaptureKeyboard;
            const bool imgui_wants_mouse = io.WantCaptureMouse;
            const bool should_forward_event =
                (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP &&
                 event.type != SDL_EVENT_TEXT_INPUT && !imgui_wants_mouse) ||
                (!imgui_wants_keyboard && !imgui_wants_mouse);

            if (should_forward_event)
            {
                crafting_states->handle_event(event);
            }
        }
    }

    void update(const float delta_time, mazes::randomizer &rng) const noexcept
    {
        // Calculate instantaneous FPS and frame time
        const auto FPS = static_cast<int>(1.0 / (delta_time / 1000.0));
        const auto frame_time = delta_time;

        // Update smoothed values periodically for display
        fps_timer += delta_time;
        if (constexpr double FPS_UPDATE_INTERVAL = 250.0; fps_timer >= FPS_UPDATE_INTERVAL)
        {
            smoothed_fps_counter = FPS;
            fps_timer_smoothed = frame_time;
            fps_timer = 0.0;
        }
        crafting_states->update(delta_time, std::ref(rng));
    }

    void render() const noexcept
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        // Apply the per-player font scale
        ImGui::GetIO().FontGlobalScale = active_player._configs.gui_font_scale();
        ImGui::PushFont(active_fonts.get(craft::craft_impl::SELECTABLE_FONTS.at(active_player.get_font_index())).get());

        crafting_states->draw();

        render_FPS();

        ImGui::PopFont();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(this->simple_direct_medialayer.window);
    }
}; // craft_impl

// Static member definitions
std::once_flag craft::craft_impl::loading_state::LOAD_RESOURCES_ONCE_FLAG;
std::vector<std::string_view> craft::craft_impl::LOADING_FONTS_WITH_NAMES{
    "Cousine Regular",
    "Limelight Regular",
    "Nunito Sans"};
std::vector<FontIdentifier> craft::craft_impl::SELECTABLE_FONTS;

craft::craft(const std::string &title, const int w, const int h)
    : crafting_impl{std::make_unique<craft_impl>(cref(title), w, h)}
{
}

craft::~craft() = default;

/**
 * Run the craft-engine in a loop with SDL window open
 */
bool craft::run([[maybe_unused]] mazes::grid_interface *g, mazes::randomizer &rng) const noexcept
{
    if (!this->crafting_impl->simple_direct_medialayer.window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL not initialized (%s)\n", SDL_GetError());
        return false;
    }

    static constexpr auto USE_DATABASE = true;
    if constexpr (USE_DATABASE)
    {
        db_enable();
        std::string db_file;
#if defined(__EMSCRIPTEN__)
        db_file = ":memory:";
#else
        db_file = "voxels.db";
#endif

        if (db_init(db_file.data()) != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Database initialization failed\n");
            return false;
        }
    }

    // LOCAL VARIABLES
    auto previous = SDL_GetTicks();
    auto last_commit = SDL_GetTicks();

    double time_step = 0.0;
    double accumulator = 0.0;

    this->crafting_impl->crafting_states->update(0.0f, std::ref(rng));

    // BEGIN EVENT LOOP
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (this->crafting_impl->active_player.is_active() && !this->crafting_impl->crafting_states->is_empty())
#endif
    {
        // FRAME RATE
        static constexpr auto FIXED_TIME_STEP = 1000.0 / 60.0;
        const auto current = SDL_GetTicks();
        const auto elapsed = static_cast<double>(current - previous);
        previous = current;
        accumulator += elapsed;

        // FLUSH DATABASE
        static constexpr auto COMMIT_INTERVAL = 5000;
        if (current - last_commit > COMMIT_INTERVAL)
        {
            db_commit();
            last_commit = current;
        }

        while (accumulator >= FIXED_TIME_STEP)
        {
            this->crafting_impl->process_input();

            time_step += FIXED_TIME_STEP;
            accumulator -= FIXED_TIME_STEP;

            this->crafting_impl->update(FIXED_TIME_STEP, std::ref(rng));
        }

        this->crafting_impl->render();

        time_step = time_step >= 1000.0 ? 0.0 : time_step;
    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    SDL_Log("Run loop ended, beginning cleanup...\n");

    SDL_Log("Clearing state stack...\n");
    if (!this->crafting_impl->crafting_states->is_empty())
    {
        this->crafting_impl->crafting_states->clear_states();
        this->crafting_impl->crafting_states->apply_pending_changes();
    }

    // Cleanup ImGui
    SDL_Log("Shutting down ImGui...\n");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // Cleanup database
    SDL_Log("Closing database...\n");
    db_close();
    db_disable();

    // Cleanup SDL (this must be last)
    SDL_Log("Shutting down SDL...\n");
    this->crafting_impl->simple_direct_medialayer.destroy_and_quit();

    SDL_Log("Cleanup complete, exiting gracefully.\n");

    return true;
} // run

std::string craft::artifacts() const noexcept
{
    return this->crafting_impl->active_player.artifacts();
}

bool craft::is_download_ready() const noexcept
{
    return this->crafting_impl->active_player.is_download_ready();
}

void craft::reset_download_flag() const noexcept
{
    this->crafting_impl->active_player._configs.artifacts_ready(false);
}

// ── Async export ─────────────────────────────────────────────────────────────

void craft::begin_export() const noexcept
{
    // Kick off the async OBJ-generation worker; safe to call from JS event handler.
    this->crafting_impl->active_player.start_async_artifact_export();
}

bool craft::is_export_ready() const noexcept
{
    // Non-blocking poll: returns true once the background OBJ worker has finished.
    return this->crafting_impl->active_player.is_artifact_export_ready();
}

std::string craft::get_export() noexcept
{
    // Retrieve the finished OBJ string; empty if called before is_export_ready().
    const std::string result = this->crafting_impl->active_player.get_artifact_export_result();
    // Mark download flag so legacy is_download_ready() path is also satisfied.
    if (!result.empty())
    {
        this->crafting_impl->active_player._configs.artifacts_ready(true);
    }
    return result;
}

std::string craft::get_export_status() const noexcept
{
    // Returns a plain string the JS layer can show in a status overlay.
    // "idle"    – no export requested yet
    // "running" – worker thread active
    // "ready"   – result available, call get_export()
    if (this->crafting_impl->active_player.is_artifact_export_ready())
    {
        return "ready";
    }
    // Check whether a future is in-flight (valid but not yet ready).
    // We reuse is_artifact_export_ready() for the ready check and infer running
    // from the download flag being false and an export having been requested.
    if (this->crafting_impl->active_player._configs.artifacts_ready() && !this->crafting_impl->active_player.is_artifact_export_ready())
    {
        return "ready";
    }
    // Distinguish "never started" vs "running" via the future validity heuristic:
    // start_async_artifact_export clears artifact_cache_results before launching.
    // If the cached result is empty and download flag is false we may be running.
    // We surface this as "running" conservatively; the JS side tolerates either.
    return "idle";
}

// ── Maze configuration (callable from JS before begin_export) ────────────────

void craft::set_maze_rows(const int rows) noexcept
{
    // Set the number of rows for the next maze generation / export.
    if (rows > 0)
    {
        this->crafting_impl->active_player._configs.maze().ensure_rows(static_cast<unsigned int>(rows));
    }
}

void craft::set_maze_columns(const int cols) noexcept
{
    // Set the number of columns for the next maze generation / export.
    if (cols > 0)
    {
        this->crafting_impl->active_player._configs.maze().ensure_columns(static_cast<unsigned int>(cols));
    }
}

void craft::set_maze_algo(const std::string &algo_name) noexcept
{
    // Accept a lowercase algorithm name ("dfs", "binary_tree", "sidewinder", …).
    try
    {
        const mazes::algo a = mazes::to_algo_from_sv(algo_name);
        this->crafting_impl->active_player._configs.maze().algo_id(a);
    }
    catch (const std::exception &ex)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "set_maze_algo: unknown algo '%s' – %s\n", algo_name.c_str(), ex.what());
    }
}

void craft::set_maze_seed(const int seed) noexcept
{
    // Seed the RNG used by the maze generator; 0 = random.
    this->crafting_impl->active_player._configs.maze().ensure_seed(static_cast<unsigned int>(seed));
}

// ── Engine meta ───────────────────────────────────────────────────────────────

std::string craft::get_version() const noexcept
{
    // Returns the MazeBuilder library version string (e.g. "8.2.1") for JS overlays.
    return mazes::buildinfo::VERSION;
}
