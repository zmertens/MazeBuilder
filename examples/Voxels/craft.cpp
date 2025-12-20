// Showcases basic voxel world and editing capabilities
// Follows game development patterns like Command Queues and State Stacks
// Creates mazes with Maze Builder library
// Originally written in C99, and I ported it to C++17

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

#include "command_queue.h"
#include "db.h"
#include "resource_manager.h"
#include "font.h"
#include "player.h"
#include "resource_identifiers.h"
#include "sdl_gl_helper.h"
#include "shader.h"
#include "texture.h"
#include "world.h"

#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>

namespace
{
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
            explicit context(SDL_Window* window, font_manager& fonts, shader_manager& shaders
                             , texture_manager& textures, player& p, sdl_gl_helper& sdl)
                : m_window{window}, m_fonts{&fonts}, m_shaders{&shaders}, m_textures{&textures}, m_player{&p},
                  m_sdl{&sdl}
            {
            }

            SDL_Window* m_window;
            font_manager* m_fonts;
            shader_manager* m_shaders;
            texture_manager* m_textures;
            player* m_player;
            sdl_gl_helper* m_sdl;
        };

        explicit state(state_stack& stack, const context& _context) : m_stack{&stack}, m_context{_context}
        {
        }

        virtual void draw() const noexcept = 0;
        virtual bool update(float delta_time, mazes::randomizer& rng) noexcept = 0;
        virtual bool handle_event(SDL_Event& event) noexcept = 0;

    protected:
        void request_stack_push(const StateIdentifier state_id) const
        {
            m_stack->push_state(state_id);
        }

        void request_stack_pop() const
        {
            m_stack->pop_state();
        }

        void request_stack_clear() const
        {
            m_stack->clear_states();
        }

        [[nodiscard]] const context& get_context() const noexcept
        {
            return m_context;
        }

        [[nodiscard]] state_stack& get_stack() const noexcept
        {
            return *m_stack;
        }

    private:
        state_stack* m_stack;
        context m_context;
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

        std::vector<state::ptr> m_stack;
        std::vector<pending_change> m_pending_list;
        state::context m_context;
        std::map<StateIdentifier, std::function<state::ptr()>> m_factories;

    public:
        explicit state_stack(const state::context& _context)
            : m_context(_context)
        {
        }

        template <typename T>
        void register_state(StateIdentifier state_id)
        {
            m_factories.insert_or_assign(state_id, [this]()
            {
                return state::ptr(std::make_unique<T>(*this, m_context));
            });
        }

        template <typename Pointer>
        [[nodiscard]] Pointer peek_state() const noexcept
        {
            auto reversed = m_stack | std::views::reverse;

            auto it = std::ranges::find_if(reversed, [](const auto& state_ptr)
            {
                return dynamic_cast<Pointer>(state_ptr.get()) != nullptr;
            });

            if (it != std::ranges::end(reversed))
            {
                return dynamic_cast<Pointer>(it->get());
            }

            return nullptr;
        }

        void update(const float delta_time, mazes::randomizer& rng) noexcept
        {
            for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
            {
                if (!(*it)->update(delta_time, std::ref(rng)))
                {
                    break;
                }
            }

            apply_pending_changes();
        }

        void draw() const noexcept
        {
            if (m_stack.empty())
            {
                return;
            }

            for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
            {
                (*it)->draw();
            }
        }

        void handle_event(SDL_Event& event) noexcept
        {
            for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
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
            m_pending_list.emplace_back(StackAction::PUSH, state_id);
        }

        void pop_state()
        {
            m_pending_list.emplace_back(StackAction::POP);
        }

        void clear_states()
        {
            m_pending_list.emplace_back(StackAction::CLEAR);
        }

        void apply_pending_changes()
        {
            for (const pending_change& change : m_pending_list)
            {
                switch (change.action)
                {
                case StackAction::PUSH:
                    m_stack.push_back(create_state(change.state_id));
                    break;
                case StackAction::POP:
                    m_stack.pop_back();
                    break;
                case StackAction::CLEAR:
                    m_stack.clear();
                    break;
                }
            }

            m_pending_list.clear();
        }

        [[nodiscard]] bool is_empty() const noexcept
        {
            return m_stack.empty();
        }

    private:
        state::ptr create_state(const StateIdentifier state_id)
        {
            if (const auto found = m_factories.find(state_id); found != m_factories.cend())
            {
                return found->second();
            }

            throw std::runtime_error("state_stack::create_state - No factory found for state ID");
        }
    }; // state_stack

    class loading_state;

    // Handles main gameplay workflow (building, editing, and rendering the voxel world)
    class editor_state final : public state
    {
        player& m_player;
        std::optional<world> m_world;

    public:
        explicit editor_state(state_stack& stack, const context& _context)
            : state{stack, _context}, m_player{*_context.m_player}
        {
        }

        ~editor_state() override
        {
            if (m_world.has_value())
            {
                m_world.reset();
            }
        }

        void draw() const noexcept override
        {
            if (m_world.has_value())
            {
                m_world->draw();
            }
        }

        bool update(const float delta_time, mazes::randomizer& rng) noexcept override
        {
            // Initialize world only after loading state has finished
            if (!m_world.has_value())
            {
                if (const auto* loading = get_stack().peek_state<loading_state*>())
                {
                    if (loading->is_finished())
                    {
                        m_world.emplace(get_context().m_window, *get_context().m_fonts,
                                        &m_player, *get_context().m_shaders, *get_context().m_textures,
                                        get_context().m_sdl);

                        m_world.value().init();

                        // Enable mouse capture for editor
                        SDL_SetWindowRelativeMouseMode(get_context().m_window, true);

                        SDL_Log("Editor: World initialized after loading completed\n");
                    }
                }
                // Loading state might already be popped, initialize if it's not in the stack
                else
                {
                    m_world.emplace(get_context().m_window,
                                    *get_context().m_fonts, &m_player,
                                    *get_context().m_shaders, *get_context().m_textures,
                                    get_context().m_sdl);

                    // Enable mouse capture for editor
                    SDL_SetWindowRelativeMouseMode(get_context().m_window, true);

                    SDL_Log("Editor: World initialized (loading state not found)\n");
                }
            }

            if (m_world.has_value())
            {
                // Re-enable mouse capture when returning from menu
                if (!SDL_GetWindowRelativeMouseMode(get_context().m_window))
                {
                    SDL_SetWindowRelativeMouseMode(get_context().m_window, true);
                }

                m_player.update(delta_time, std::ref(rng));
                m_world->update(delta_time, std::ref(rng));

                auto& commands = m_world->get_command_queue();
                m_player.handle_realtime_input(std::ref(commands));
            }

            return true;
        }

        bool handle_event(SDL_Event& event) noexcept override
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                SDL_Log("Editor: Received SDL_QUIT event - clearing stack\n");
                m_player.set_active(false);
                return false;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    SDL_SetWindowRelativeMouseMode(get_context().m_window, false);
                    request_stack_push(StateIdentifier::MENU);
                    return false;
                }
                break;

            default:
                break;
            }

            if (m_world.has_value())
            {
                auto& commands = m_world->get_command_queue();
                m_player.handle_event(event, std::ref(commands));
                m_world->handle_event(event);
            }

            return true;
        } // handle_event
    }; // editor_state

    class loading_state final : public state
    {
        // Static once_flag to ensure load_resources is called only once across all instances
        static std::once_flag s_load_resources_flag;

        void load_resources() const noexcept
        {
            // fonts
            static constexpr auto FONT_PIXEL_SIZE = 28.f;

            auto&& fonts = get_context().m_fonts;

            fonts->load(FontIdentifier::COUSINE_REGULAR, Cousine_Regular_compressed_data,
                        Cousine_Regular_compressed_size, FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::LIMELIGHT, Limelight_Regular_compressed_data, Limelight_Regular_compressed_size,
                        FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::NUNITO_SANS, NunitoSans_compressed_data, NunitoSans_compressed_size,
                        FONT_PIXEL_SIZE);

            // shaders
            std::vector<std::tuple<ShaderIdentifier, std::string, std::string>> shader_programs{
                {
                    ShaderIdentifier::BLOCK_SHADER,
                    "shaders/block_vertex.glsl",
                    "shaders/block_fragment.glsl"
                },
                {
                    ShaderIdentifier::LINE_SHADER,
                    "shaders/line_vertex.glsl",
                    "shaders/line_fragment.glsl"
                },
                {
                    ShaderIdentifier::SKY_SHADER,
                    "shaders/sky_vertex.glsl",
                    "shaders/sky_fragment.glsl"
                },
                {
                    ShaderIdentifier::TEXT_SHADER,
                    "shaders/text_vertex.glsl",
                    "shaders/text_fragment.glsl"
                }
            };

            std::vector<std::tuple<ShaderIdentifier, std::string, std::string>> shader_gles_programs{
                {
                    ShaderIdentifier::BLOCK_SHADER,
                    "shaders/es/block_vertex.es.glsl",
                    "shaders/es/block_fragment.es.glsl"
                },
                {
                    ShaderIdentifier::LINE_SHADER,
                    "shaders/es/line_vertex.es.glsl",
                    "shaders/es/line_fragment.es.glsl"
                },
                {
                    ShaderIdentifier::SKY_SHADER,
                    "shaders/es/sky_vertex.es.glsl",
                    "shaders/es/sky_fragment.es.glsl"
                },
                {
                    ShaderIdentifier::TEXT_SHADER,
                    "shaders/es/text_vertex.es.glsl",
                    "shaders/es/text_fragment.es.glsl"
                }
            };

#if defined(__EMSCRIPTEN__)
            for (const auto& [id, vertex_path, fragment_path] : shader_gles_programs)
#else
            for (const auto& [id, vertex_path, fragment_path] : shader_programs)
#endif
            {
                get_context().m_shaders->load(id, vertex_path, fragment_path);

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

            auto&& textures = get_context().m_textures;

            textures->load(TextureIdentifier::ATLAS, atlas_path, static_cast<unsigned int>(TextureIdentifier::ATLAS));
            textures->load(TextureIdentifier::BITMAP_FONT, bitmap_font_path,
                static_cast<unsigned int>(TextureIdentifier::BITMAP_FONT));

            // Initialize MAZE texture with 1x1 white pixel placeholder
            constexpr std::uint8_t white_pixel[4] = { 255, 255, 255, 255 };
            textures->load(TextureIdentifier::MAZE, 1, 1, white_pixel,
                static_cast<unsigned int>(TextureIdentifier::MAZE));

            textures->load(TextureIdentifier::SIGNS, signs_path, static_cast<unsigned int>(TextureIdentifier::SIGNS));
            textures->load(TextureIdentifier::SKY, sky_path, static_cast<unsigned int>(TextureIdentifier::SKY));
            textures->load(get_context().m_window, TextureIdentifier::WINDOW_ICON, window_icon_path);

#if defined(MAZE_DEBUG)

            std::ranges::for_each(s_font_names, [](const auto& name)
            {
                SDL_Log("Loaded font: %s\n", name.data());
            });

            SDL_Log("Loaded textures\n%s\n%s\n%s\n%s\n%s\n", atlas_path.data(),
                    bitmap_font_path.data(), window_icon_path.data(), signs_path.data(), sky_path.data());
#endif
        } // load_resources

        bool m_has_finished{false};

    public:
        explicit loading_state(state_stack& _stack, const context& _context)
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

                const char* status_text = m_has_finished ? "Complete!" : "Please wait...";
                const float status_width = ImGui::CalcTextSize(status_text).x;
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - status_width) * 0.5f);
                ImGui::TextColored(ImVec4(0.933f, 1.0f, 0.8f, 1.0f), "%s", status_text);
            }
            ImGui::End();

            ImGui::PopStyleColor(4);
        }

        bool update(const float delta_time, mazes::randomizer& rng) noexcept override
        {
            if (!m_has_finished)
            {
                std::call_once(s_load_resources_flag, [this]()
                {
                    load_resources();
                });

                m_has_finished = true;
                request_stack_pop();
            }

            return true;
        }

        bool handle_event(SDL_Event& event) noexcept override
        {
            return true;
        }

        [[nodiscard]] bool is_finished() const noexcept
        {
            return m_has_finished;
        }
    };

    // Handles GUI options
    class menu_state final : public state
    {
        std::vector<FontIdentifier> m_selectable_fonts;
        std::size_t m_selected_font_index{0};
        std::list<std::string> algo_list;

    public:
        explicit menu_state(state_stack& stack, const context& context)
            : state{stack, context}
        {
            m_selectable_fonts.reserve(static_cast<std::size_t>(FontIdentifier::TOTAL));
            std::ranges::for_each(
                std::views::iota(0, static_cast<int>(FontIdentifier::TOTAL)),
                [this](const int id)
                {
                    m_selectable_fonts.push_back(static_cast<FontIdentifier>(id));
                    return true;
                });
            for (auto i{ static_cast<int>(mazes::algo::BINARY_TREE) }; i < static_cast<int>(mazes::algo::TOTAL); ++i) {
                algo_list.emplace_back(mazes::to_sv_from_algo(static_cast<mazes::algo>(i)));
            }
        }

        void draw() const noexcept override
        {
            auto&& current_configs = get_context().m_player->m_configs;
            auto* p = get_context().m_player;
            static auto selected_font_index{0};
            ImGui::PushFont(get_context().m_fonts->get(m_selectable_fonts.at(selected_font_index)).get());

            // Apply color schema
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.95f));
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
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.933f, 1.0f, 0.8f, 1.0f));

            // Center the modal window
            const ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::SetNextWindowBgAlpha(0.95f);

            constexpr ImGuiWindowFlags window_flags =
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings;

            ImGui::OpenPopup("Mazes");

            if (ImGui::BeginPopupModal("Mazes", nullptr, window_flags))
            {
                if (ImGui::BeginTabBar("MenuTabs"))
                {
                    if (ImGui::BeginTabItem("Main"))
                    {
                        if (current_configs.download_ready && ImGui::Button("Download mazes",
                            ImVec2(220, 40)))
                        {
                            handle_artifacts(p);
                        }
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f),
                                           "Navigation Options:");
                        ImGui::Spacing();
                        if (ImGui::Button("Resume", ImVec2(220, 40)))
                        {
                            request_stack_pop();
                        }
                        ImGui::Separator();
                        ImGui::Spacing();
                        if (ImGui::Button("New Editor", ImVec2(220, 40)))
                        {
                            // Start a new editor session
                            // @TODO : new DB
                            request_stack_clear();
                            request_stack_push(StateIdentifier::EDITOR);
                            request_stack_push(StateIdentifier::LOADING);
                        }
                        ImGui::Separator();
                        ImGui::Spacing();
                        if (ImGui::Button("Close Application", ImVec2(270, 40)))
                        {
                            request_stack_clear();
                        }
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Commands:");
                        ImGui::Spacing();
                        ImGui::Text("Left Mouse Click: Delete a block");
                        ImGui::Text("Right Mouse Click: Build a block");
                        ImGui::Text("Middle Mouse Click: Copy selected block type");
                        ImGui::Text("Middle Mouse Scroll: Cycle block types");
                        ImGui::Text("Spacebar: Jump");
                        ImGui::Text("Tab: Fly");
                        ImGui::Text("Left Shift: Hover up while flying");
                        ImGui::Text("F: Hover down while flying");
                        ImGui::Text("Q: Auto run Movement");
                        ImGui::Text("W: Forward Movement");
                        ImGui::Text("A: Left Movement");
                        ImGui::Text("S: Backward Movement");
                        ImGui::Text("D: Right Movement");
                        ImGui::Text("B: Build maze at cursor");
                        ImGui::Text("E: Preview maze at cursor");
                        ImGui::Text("T: Tag a block");
                        ImGui::Text("Left Control: Place light");

                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Builder"))
                    {
                        ImGui::Text("Maze Configuration");
                        ImGui::Separator();
                        ImGui::Spacing();

                        auto&& maze_config = get_context().m_player->m_configs.maze;

                        static std::string selected_algo = "";
                        selected_algo = mazes::to_sv_from_algo(maze_config.algo_id());
                        static int rows = static_cast<int>(maze_config.rows());
                        static int columns = static_cast<int>(maze_config.columns());
                        static int levels = static_cast<int>(maze_config.levels());
                        static int seed = static_cast<int>(maze_config.seed());

                        ImGui::SliderInt("Rows", &rows, 2, 10);
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::SliderInt("Columns", &columns, 2, 10);
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::SliderInt("Levels", &levels, 1, 5);
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (ImGui::TreeNode("Algo")) {
                            auto preview{ selected_algo };
                            ImGui::NewLine();
                            if (constexpr ImGuiComboFlags combo_flags = ImGuiComboFlags_PopupAlignLeft |
                                ImGuiComboFlags_WidthFitPreview;
                                ImGui::BeginCombo("algorithm", preview.data(), combo_flags)) {
                                for (const auto& itr : algo_list) {
                                    const bool is_selected = (mazes::to_algo_from_sv(itr) == maze_config.algo_id());
                                    if (ImGui::Selectable(std::string{itr}.c_str(), is_selected)) {
                                        maze_config.algo_id(mazes::to_algo_from_sv(itr));
                                        selected_algo = itr;
                                    }
                                    if (is_selected)
                                    {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::NewLine();
                            ImGui::TreePop();
                        }

                        ImGui::SliderInt("Seed", &seed, 0, 1000000);
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Checkbox("Preview Enabled", &get_context().m_player->m_configs.preview_enabled);
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Instructions:");
                        ImGui::Text("1. Configure maze parameters above");
                        ImGui::Text("2. Press 'Apply Configs' button");
                        ImGui::Text("3. Press 'E' key in editor to generate preview");
                        ImGui::Text("4. Aim at a block face");
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (ImGui::Button("Apply Configs", ImVec2(200, 40)))
                        {
                            get_context().m_player->m_configs.maze
                                         .algo_id(mazes::to_algo_from_sv(selected_algo))
                                         .rows(static_cast<unsigned int>(rows))
                                         .columns(static_cast<unsigned int>(columns))
                                         .levels(static_cast<unsigned int>(levels))
                                         .seed(static_cast<unsigned int>(seed));
                        }
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Settings"))
                    {
                        static bool toggle_color_mode = false;
                        ImGui::Checkbox("Toggle Dark Mode", &toggle_color_mode);
                        if (toggle_color_mode)
                        {
                            ImGui::StyleColorsLight();
                        }
                        else
                        {
                            ImGui::StyleColorsDark();
                        }

                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Font Selection:");
                        ImGui::Spacing();
                        if (ImGui::BeginListBox("##FontListBox", ImVec2(-100, 200)))
                        {
                            for (std::size_t i = 0; i < m_selectable_fonts.size(); ++i)
                            {
                                const bool is_selected = (selected_font_index == i);
                                const auto& font_name = craft_impl::s_font_names.at(
                                    static_cast<std::size_t>(m_selectable_fonts.at(i)));
                                if (ImGui::Selectable(font_name.data(), is_selected))
                                {
                                    selected_font_index = i;
                                }
                                if (is_selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndListBox();
                        }

                        ImGui::Separator();
                        ImGui::Spacing();

                        auto&& current_configs = get_context().m_player->m_configs;

                        const auto last_vsync = current_configs.vsync;
                        const auto last_fullscreen = current_configs.fullscreen;

                        ImGui::SliderFloat("FoV", &current_configs.fov, 30, 120, "%.3f degrees");
                        ImGui::Checkbox("Show Stats Overlay", &current_configs.show_stats_window);
                        ImGui::Checkbox("Enable VSync", &current_configs.vsync);
                        ImGui::Checkbox("Enable fullscreen", &current_configs.fullscreen);
                        ImGui::Checkbox("Invert Mouse Y-Axis", &current_configs.invert_mouse);
                        ImGui::SliderInt("Orthographic scaling", &current_configs.ortho, 0, 64);
                        // ImGui::Checkbox("Apply Bloom Effect", &current_configs.use_bloom_effect);
                        // ImGui::SliderFloat("Exp", &current_configs.exposure_range, 0.1f, 1.0f, "%.2f");

                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Message:");
                        ImGui::Spacing();

                        static char tag_buffer[256] = "";

                        // Copy current tag to buffer on first use or when changed externally
                        static bool initialized = false;
                        if (!initialized || SDL_strcmp(tag_buffer, current_configs.tag.c_str()) != 0)
                        {
                            SDL_strlcpy(tag_buffer, current_configs.tag.c_str(), SDL_arraysize(tag_buffer));
                            tag_buffer[SDL_arraysize(tag_buffer) - 1] = '\0';
                            initialized = true;
                        }

                        if (ImGui::InputText("##PlayerTag", tag_buffer, std::size(tag_buffer)))
                        {
                            current_configs.tag = std::string(tag_buffer);
                        }

                        ImGui::Separator();
                        ImGui::Spacing();

                        if (last_vsync != current_configs.vsync)
                        {
                            SDL_GL_SetSwapInterval(current_configs.vsync ? 1 : 0);
                        }

                        if (last_fullscreen != current_configs.fullscreen)
                        {
                            SDL_SetWindowFullscreen(get_context().m_window, current_configs.fullscreen);
                        }

                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(17);
            ImGui::PopFont();
        }

        bool update(float delta_time, mazes::randomizer& rng) noexcept override
        {
            return false;
        }

        bool handle_event(SDL_Event& event) noexcept override
        {
            if (SDL_EVENT_QUIT == event.type)
            {
                SDL_Log("Menu: Received SDL_QUIT event - clearing states\n");
                get_context().m_player->set_active(false);
                request_stack_clear();
            }

            // Handle ESCAPE to return to game
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                SDL_Log("Menu: ESCAPE pressed - returning to editor\n");
                request_stack_pop();
            }

            return false;
        }
    }; // menu_state

    const std::string& INIT_WINDOW_TITLE;
    const int INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT;

    bool m_show_download_button{false};

    std::unique_ptr<state_stack> m_crafting_states;

    static std::vector<std::string_view> s_font_names;

    font_manager m_fonts;
    shader_manager m_shaders;
    texture_manager m_textures;

    player m_player;

    sdl_gl_helper m_sdl;

    mazes::grid_factory m_grid_factory;

    mutable double m_fps_update_timer{0.0};
    mutable int m_smoothed_fps{0};
    mutable float m_smoothed_frame_time{0.0f};

    craft_impl(const std::string& title, const int w, const int h)
        : INIT_WINDOW_TITLE(title), INIT_WINDOW_WIDTH(w), INIT_WINDOW_HEIGHT(h)
    {
        start_SDL();

        setup_imgui();

        m_grid_factory.register_creator(
            title, [](const mazes::configurator& config) -> std::unique_ptr<mazes::grid_interface>
            {
                return std::make_unique<mazes::grid>(config.rows(), config.columns(), config.levels());
            });

        m_crafting_states = std::make_unique<state_stack>(state::context{
            m_sdl.window,
            std::ref(m_fonts),
            std::ref(m_shaders),
            std::ref(m_textures),
            std::ref(m_player),
            std::ref(m_sdl)
        });

        register_states();

        m_crafting_states->push_state(StateIdentifier::EDITOR);
        m_crafting_states->push_state(StateIdentifier::LOADING);
    }

    void register_states() const noexcept
    {
        m_crafting_states->register_state<editor_state>(StateIdentifier::EDITOR);
        m_crafting_states->register_state<loading_state>(StateIdentifier::LOADING);
        m_crafting_states->register_state<menu_state>(StateIdentifier::MENU);
    }

    void start_SDL() noexcept
    {
        if (m_sdl.initialize(INIT_WINDOW_TITLE, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT))
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

        ImGui_ImplSDL3_InitForOpenGL(m_sdl.window, m_sdl.gl_context);

        std::string glsl_version;

#if defined(__EMSCRIPTEN__)
        glsl_version = "#version 100";
#else
        glsl_version = "#version 130";
#endif

        ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    }

    static std::string make_filename(const player* p) noexcept
    {
        std::vector<std::string> timestamp_parts;
        const auto& itr = mazes::string_utils::split(p->get_local_time().begin(),
            p->get_local_time().end(), timestamp_parts, ':');
        const auto timestamp = mazes::string_utils::format("{}_{}",
            timestamp_parts.at(0), timestamp_parts.at(2));
        const auto rows = std::to_string(p->m_configs.maze.rows());
        const auto columns = std::to_string(p->m_configs.maze.columns());
        const auto levels = std::to_string(p->m_configs.maze.levels());

        std::string filename;
        filename.reserve(128);
        filename = rows + "x" + columns + "x" + levels + "_" +
                   p->get_name() + "_" + std::string(timestamp) + ".obj";

        return filename;
    }

    static void handle_artifacts(player* p) noexcept
    {
#if !defined(__EMSCRIPTEN__)
        const auto artifacts = p->artifacts();

        if (artifacts.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                "Failed to generate artifacts.\n");
            p->m_configs.download_ready = false;
            return ;
        }

        constexpr mazes::io_utils io_things{};
        const auto filename = make_filename(p);
        const auto success = io_things.write_file(filename, artifacts);
        p->m_configs.download_ready = !success;
        SDL_Log("Write file '%s': %s (%zu bytes)\n",
                filename.c_str(),
                success ? "SUCCESS" : "FAILED",
                artifacts.size());

#else
        SDL_Log("Web detected: download via calling for artifacts");
        p->m_configs.show_download_button = false;
#endif
    }

    void render_FPS(const double elapsed) const noexcept
    {
        // Calculate instantaneous FPS and frame time
        const auto fps = static_cast<int>(1000.0 / elapsed);
        const auto frame_time = static_cast<float>(elapsed);

        // Update smoothed values periodically for display
        m_fps_update_timer += elapsed;
        if (constexpr double FPS_UPDATE_INTERVAL = 250.0; m_fps_update_timer >= FPS_UPDATE_INTERVAL)
        {
            m_smoothed_fps = fps;
            m_smoothed_frame_time = frame_time;
            m_fps_update_timer = 0.0;
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always,
                                ImVec2(1.0f, 0.0f));

        ImGui::SetNextWindowBgAlpha(0.65f);

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove;

        if (this->m_player.m_configs.show_stats_window
            && ImGui::Begin("FPS Overlay", nullptr, windowFlags))
        {
            ImGui::Text("FPS: %d", m_smoothed_fps);
            ImGui::Text("Frame Time: %.2f ms", m_smoothed_frame_time);
            ImGui::Text("local time: %s\n", this->m_player.get_local_time().data());
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
                m_crafting_states->handle_event(event);
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                m_crafting_states->handle_event(event);
                continue;
            }

            // Check if ImGui wants to capture this event
            const ImGuiIO& io = ImGui::GetIO();
            const bool imgui_wants_keyboard = io.WantCaptureKeyboard;
            const bool imgui_wants_mouse = io.WantCaptureMouse;
            const bool should_forward_event =
                (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP &&
                    event.type != SDL_EVENT_TEXT_INPUT && !imgui_wants_mouse) ||
                (!imgui_wants_keyboard && !imgui_wants_mouse);

            if (should_forward_event)
            {
                m_crafting_states->handle_event(event);
            }
        }
    }

    void update(const float delta_time, mazes::randomizer& rng) const noexcept
    {
        m_crafting_states->update(delta_time, std::ref(rng));
    }

    void render(const double elapsed) const noexcept
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        m_crafting_states->draw();

        render_FPS(elapsed);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(this->m_sdl.window);
    }
}; // craft_impl

// Static member definitions
std::once_flag craft::craft_impl::loading_state::s_load_resources_flag;
std::vector<std::string_view> craft::craft_impl::s_font_names{
    "Cousine Regular",
    "Limelight Regular",
    "Nunito Sans"
};

craft::craft(const std::string& title, const int w, const int h)
    : m_impl{std::make_unique<craft_impl>(cref(title), w, h)}
{
}

craft::~craft() = default;

/**
 * Run the craft-engine in a loop with SDL window open
 */
bool craft::run([[maybe_unused]] mazes::grid_interface* g, mazes::randomizer& rng) const noexcept
{
    if (!this->m_impl->m_sdl.window)
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
        db_file = "craft.db";
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

    this->m_impl->m_crafting_states->update(0.0f, std::ref(rng));

    // BEGIN EVENT LOOP
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (this->m_impl->m_player.is_active() && !this->m_impl->m_crafting_states->is_empty())
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
            this->m_impl->process_input();

            time_step += FIXED_TIME_STEP;
            accumulator -= FIXED_TIME_STEP;

            this->m_impl->update(FIXED_TIME_STEP, std::ref(rng));
        }

        this->m_impl->render(elapsed);

        time_step = time_step >= 1000.0 ? 0.0 : time_step;
    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    SDL_Log("Run loop ended, beginning cleanup...\n");

    SDL_Log("Clearing state stack...\n");
    if (!this->m_impl->m_crafting_states->is_empty())
    {
        this->m_impl->m_crafting_states->clear_states();
        this->m_impl->m_crafting_states->apply_pending_changes();
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
    this->m_impl->m_sdl.destroy_and_quit();

    SDL_Log("Cleanup complete, exiting gracefully.\n");

    return true;
} // run

std::string craft::artifacts() const noexcept
{
    return this->m_impl->m_player.artifacts();
}

bool craft::is_download_ready() const noexcept
{
    return this->m_impl->m_player.m_configs.download_ready;
}

void craft::set_download_ready(const bool ready) const noexcept
{
    this->m_impl->m_player.m_configs.download_ready = ready;
}
