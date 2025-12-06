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
#include <emscripten_local/emscripten_mainloop_stub.h>
#else
#endif

#include <SDL3/SDL.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "command_queue.h"
#include "db.h"
#include "resource_manager.h"
#include "font.h"
#include "item.h"
#include "map.h"
#include "player.h"
#include "resource_identifiers.h"
#include "sdl_helper.h"
#include "texture.h"
#include "world.h"

#include <array>
#include <functional>
#include <memory>
#include <map>
#include <optional>

namespace
{
    enum class MenuItem : unsigned int
    {
        CONTINUE = 0,
        NEW_GAME = 1,
        SETTINGS = 2,
        SPLASH = 3,
        QUIT = 4,
        COUNT = 5
    };

    enum class StackAction : unsigned int
    {
        PUSH = 0,
        POP = 1,
        CLEAR = 2,
    };

    enum class StateIdentifier : unsigned int
    {
        DONE = 0,
        EDITOR = 1,
        LOADING = 2,
        MENU = 3,
    };
}

// Implement the states used in the voxel engine when running
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
            explicit context(SDL_Window* window, font_manager& fonts, texture_manager& textures, player& p)
                : m_window{window}, m_fonts{&fonts}, m_textures{&textures}, m_player{&p}
            {
            }

            SDL_Window* m_window;
            font_manager* m_fonts;
            texture_manager* m_textures;
            player* m_player;
        };

        explicit state(state_stack& stack, context context) : m_stack{&stack}, m_context{context}
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

        [[nodiscard]] context get_context() const noexcept
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
            : m_stack(), m_pending_list(), m_context(_context), m_factories()
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
        Pointer peek_state() const noexcept
        {
            for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
            {
                // @TODO C++20: use std::ranges::find_if / constexpr
                if (auto state_ptr = dynamic_cast<Pointer>((*it).get()))
                {
                    return state_ptr;
                }
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
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Log("State Stack: Handling KEY_DOWN event (stack size: %zu)\n", m_stack.size());
            }

            for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
            {
                if (!(*it)->handle_event(event))
                {
                    if (event.type == SDL_EVENT_KEY_DOWN)
                    {
                        SDL_Log("State Stack: State returned false, stopping propagation\n");
                    }
                    break;
                }
            }

            apply_pending_changes();
        }

        void push_state(StateIdentifier state_id)
        {
            SDL_Log("State Stack: Queuing PUSH for state ID %d\n", static_cast<int>(state_id));
            m_pending_list.emplace_back(StackAction::PUSH, state_id);
        }

        void pop_state()
        {
            SDL_Log("State Stack: Queuing POP\n");
            m_pending_list.emplace_back(StackAction::POP);
        }

        void clear_states()
        {
            SDL_Log("State Stack: Queuing CLEAR\n");
            m_pending_list.emplace_back(StackAction::CLEAR);
        }

        [[nodiscard]] bool is_empty() const noexcept
        {
            return m_stack.empty();
        }

    private:
        state::ptr create_state(StateIdentifier state_id)
        {
            if (const auto found = m_factories.find(state_id); found != m_factories.cend())
            {
                return found->second();
            }

            throw std::runtime_error("StateStack::createState - No factory found for state ID");
        }

        void apply_pending_changes()
        {
            if (!m_pending_list.empty())
            {
                SDL_Log("State Stack: Applying %zu pending change(s)\n", m_pending_list.size());
            }

            for (const pending_change& change : m_pending_list)
            {
                switch (change.action)
                {
                case StackAction::PUSH:
                    SDL_Log("State Stack: PUSHING state ID %d (stack size: %zu -> %zu)\n",
                            static_cast<int>(change.state_id), m_stack.size(), m_stack.size() + 1);
                    m_stack.push_back(create_state(change.state_id));
                    break;
                case StackAction::POP:
                    SDL_Log("State Stack: POPPING state (stack size: %zu -> %zu)\n",
                            m_stack.size(), m_stack.size() - 1);
                    m_stack.pop_back();
                    break;
                case StackAction::CLEAR:
                    SDL_Log("State Stack: CLEARING all states (stack size: %zu -> 0)\n", m_stack.size());
                    m_stack.clear();
                    break;
                }
            }

            m_pending_list.clear();
        }
    }; // state_stack

    // Forward declarations
    class loading_state;

    // Handles main gameplay workflow (building, editing, and rendering the voxel world)
    class editor_state final : public state
    {
    private:
        player& m_player;
        std::optional<world> m_world;
        float m_mouse_movement{};

    public:
        explicit editor_state(state_stack& stack, const context& _context)
            : state{stack, _context}, m_player{*_context.m_player}
        {
        }

        void draw() const noexcept override
        {
            if (m_world.has_value())
            {
                m_world->draw();
            }
            else
            {
                // Show "Initializing World..." message while waiting for resources (uses default font)
                // This screen is only visible for one frame after loading completes
                ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(350, 150), ImGuiCond_Always);

                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.95f));
                ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.067f, 0.137f, 0.094f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.118f, 0.227f, 0.161f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.745f, 0.863f, 0.498f, 1.0f));

                if (ImGui::Begin("Initializing", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
                {
                    ImGui::Spacing();
                    const char* init_text = "Initializing World...";
                    float text_width = ImGui::CalcTextSize(init_text).x;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_width) * 0.5f);
                    ImGui::Text("%s", init_text);

                    ImGui::Spacing();
                    ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(-1, 0), "");
                }
                ImGui::End();

                ImGui::PopStyleColor(4);
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
                        m_world.emplace(get_context().m_window, *get_context().m_fonts, *get_context().m_textures);

                        // Enable mouse capture for editor
                        SDL_SetWindowRelativeMouseMode(get_context().m_window, true);

                        SDL_Log("Editor: World initialized after loading completed\n");
                    }
                }
                // Loading state might already be popped, initialize if it's not in the stack
                else
                {
                    m_world.emplace(get_context().m_window, *get_context().m_fonts, *get_context().m_textures);

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

                m_world->update(delta_time, std::ref(rng));

                auto& commands = m_world->get_command_queue();
                m_player.handle_realtime_input(std::ref(commands));
            }

            return true;
        }

        bool handle_event(SDL_Event& event) noexcept override
        {
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Log("Editor: Received KEY_DOWN event, scancode=%d, world_initialized=%s\n",
                        event.key.scancode, m_world.has_value() ? "true" : "false");
            }

            // Handle state-specific events FIRST before passing to player/world
            // This ensures ESCAPE is not consumed by game logic
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                request_stack_clear();
                return false;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    SDL_Log("Editor: ESCAPE pressed - opening menu\n");
                    // Disable mouse capture when going to menu
                    SDL_SetWindowRelativeMouseMode(get_context().m_window, false);
                    request_stack_push(StateIdentifier::MENU);
                    return false;
                }
                break;

            default:
                break;
            }

            // Pass other events to world/player if world is initialized
            if (m_world.has_value())
            {
                auto& commands = m_world->get_command_queue();
                m_player.handle_event(event, std::ref(commands));
                m_world->handle_event(event);
            }

            return true; // Continue event propagation to lower states
        } // handle_events_and_motion
    }; // editor_state

    class loading_state final : public state
    {
        void load_resources() const noexcept
        {
            static constexpr auto FONT_PIXEL_SIZE = 28.f;

            auto&& fonts = get_context().m_fonts;

            fonts->load(FontIdentifier::COUSINE_REGULAR, Cousine_Regular_compressed_data,
                        Cousine_Regular_compressed_size, FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::LIMELIGHT, Limelight_Regular_compressed_data, Limelight_Regular_compressed_size,
                        FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::NUNITO_SANS, NunitoSans_compressed_data, NunitoSans_compressed_size,
                        FONT_PIXEL_SIZE);

            constexpr std::string_view atlas_path = "textures/atlas.png";
            constexpr std::string_view bitmap_font_path = "textures/bitmap_font.png";
            constexpr std::string_view window_icon_path = "textures/icon.bmp";
            constexpr std::string_view signs_path = "textures/signs.png";

            auto&& textures = get_context().m_textures;

            textures->load(TextureIdentifier::ATLAS, atlas_path, 0);
            textures->load(TextureIdentifier::BITMAP_FONT, bitmap_font_path, 1);
            textures->load(get_context().m_window, TextureIdentifier::WINDOW_ICON, window_icon_path);
            textures->load(TextureIdentifier::SIGNS, signs_path, 2);

#if defined(MAZE_DEBUG)

            SDL_Log("Loaded fonts\nCousine Regular\nLimelight Regular\nNunito Sans\n");

            SDL_Log("Loaded textures\n%s\n%s\n%s\n%s\n", atlas_path.data(),
                    bitmap_font_path.data(), window_icon_path.data(), signs_path.data());
#endif
        }

        bool m_has_finished;

    public:
        explicit loading_state(state_stack& _stack, state::context _context)
            : state(_stack, _context), m_has_finished{false}
        {
        }

        void draw() const noexcept override
        {
            // Display loading screen with ImGui (using default font since custom fonts aren't loaded yet)

            // Center the loading window
            ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);

            // Style the loading window
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.067f, 0.137f, 0.094f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.118f, 0.227f, 0.161f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.745f, 0.863f, 0.498f, 1.0f));

            if (ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
            {
                ImGui::Spacing();
                ImGui::Spacing();

                // Center the text
                const char* loading_text = "Loading Resources...";
                float text_width = ImGui::CalcTextSize(loading_text).x;
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_width) * 0.5f);
                ImGui::Text("%s", loading_text);

                ImGui::Spacing();
                ImGui::Spacing();

                // Show a simple progress bar or spinner effect
                ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(-1, 0), "");

                ImGui::Spacing();

                const char* status_text = m_has_finished ? "Complete!" : "Please wait...";
                float status_width = ImGui::CalcTextSize(status_text).x;
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
                load_resources();
                m_has_finished = true;

                // Pop loading state once loading is complete
                // Editor state is already on the stack below us
                SDL_Log("Loading: Resources loaded, popping loading state\n");
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
    public:
        explicit menu_state(state_stack& stack, const context& context)
            : state{stack, context}
        {
        }

        void draw() const noexcept override
        {
            ImGui::PushFont(get_context().m_fonts->get(FontIdentifier::LIMELIGHT).get());

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
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.933f, 1.0f, 0.8f, 1.0f));

            ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_Modal))
            {
                ImGui::Text("Welcome to MazeBuilder Physics");
                ImGui::Separator();
                ImGui::Spacing();

                // Navigation options
                ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Navigation Options:");
                ImGui::Spacing();

                // Resume button - return to game
                if (ImGui::Button("Resume", ImVec2(200, 40)))
                {
                    request_stack_pop();
                }
                ImGui::Spacing();

                // New Game button - placeholder for now
                if (ImGui::Button("New Game", ImVec2(200, 40)))
                {
                    // TODO: Implement new game functionality
                    SDL_Log("New Game - Not yet implemented\n");
                }
                ImGui::Spacing();

                // Settings button - placeholder for now
                if (ImGui::Button("Settings", ImVec2(200, 40)))
                {
                    // TODO: Implement settings functionality
                    SDL_Log("Settings - Not yet implemented\n");
                }
                ImGui::Spacing();

                // Splash Screen button - placeholder for now
                if (ImGui::Button("Splash Screen", ImVec2(200, 40)))
                {
                    // TODO: Implement splash screen functionality
                    SDL_Log("Splash Screen - Not yet implemented\n");
                }
                ImGui::Spacing();

                // Quit button - exit application
                if (ImGui::Button("Quit", ImVec2(200, 40)))
                {
                    request_stack_clear();
                }

                ImGui::Separator();
                ImGui::Spacing();

                // Display controls help
                ImGui::TextColored(ImVec4(0.933f, 1.0f, 0.8f, 1.0f), "Press ESC to return to game");
            }

            ImGui::End();

            ImGui::PopStyleColor(10);

            ImGui::PopFont();
        }

        bool update(float delta_time, mazes::randomizer& rng) noexcept override
        {
            // Pause underlying states (editor) while menu is active
            return false;
        }

        bool handle_event(SDL_Event& event) noexcept override
        {
            // Let ImGui handle all events while in menu
            return false;
        }
    }; // menu_state

    const std::string& INIT_WINDOW_TITLE;
    const int INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT;

    std::unique_ptr<state_stack> m_crafting_states;

    font_manager m_fonts;
    texture_manager m_textures;

    player m_player;

    sdl_helper m_sdl;

    mutable double fps_update_timer{0.0};
    mutable int smoothed_fps{0};
    mutable float smoothed_frame_time{0.0f};

    craft_impl(const std::string& title, const int w, const int h)
        : INIT_WINDOW_TITLE(title), INIT_WINDOW_WIDTH(w), INIT_WINDOW_HEIGHT(h)
    {
        start_SDL();

        setup_imgui();

        m_crafting_states = std::make_unique<state_stack>(state::context{
            m_sdl.window, std::ref(m_fonts), std::ref(m_textures), std::ref(m_player)
        });

        register_states();

        // Push editor first (bottom of stack), then loading (top of stack)
        // States are drawn/updated in reverse order (rbegin to rend)
        // So loading will be processed first and draw on top
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
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
        ImGui::GetIO().IniFilename = nullptr;

        // Setup ImGui Platform/Renderer backends
        ImGui_ImplSDL3_InitForOpenGL(m_sdl.window, m_sdl.gl_context);

        std::string glsl_version;

#if defined(__EMSCRIPTEN__)
        glsl_version = "#version 100";
#else
        glsl_version = "#version 130";
#endif

        ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    }

    void handle_FPS(double& time_step, const double elapsed) const noexcept
    {
        // Calculate instantaneous FPS and frame time
        const auto fps = static_cast<int>(1000.0 / elapsed);
        const auto frame_time = static_cast<float>(elapsed);

        // Update smoothed values periodically for display
        fps_update_timer += elapsed;
        if (constexpr double FPS_UPDATE_INTERVAL = 250.0; fps_update_timer >= FPS_UPDATE_INTERVAL)
        {
            smoothed_fps = fps;
            smoothed_frame_time = frame_time;
            fps_update_timer = 0.0;
        }

        if (time_step >= 1000.0)
        {
            SDL_Log("FPS: %d\n", smoothed_fps);
            SDL_Log("Frame Time: %.3f ms/frame\n", smoothed_frame_time);

            time_step = 0.0;
        }

        // Create ImGui overlay window
        // Set window position to top-right corner
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always,
                                ImVec2(1.0f, 0.0f));

        // Set window background to be semi-transparent
        ImGui::SetNextWindowBgAlpha(0.65f);

        // Create window with no title bar, no resize, no move, auto-resize
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("FPS Overlay", nullptr, windowFlags))
        {
            ImGui::Text("FPS: %d", smoothed_fps);
            ImGui::Text("Frame Time: %.2f ms", smoothed_frame_time);
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
                SDL_Log("Received SDL_QUIT event. Exiting main loop.\n");
                m_crafting_states->clear_states();
                break;
            }

            // Check if ImGui wants to capture this event
            ImGuiIO& io = ImGui::GetIO();
            bool imgui_wants_keyboard = io.WantCaptureKeyboard;
            bool imgui_wants_mouse = io.WantCaptureMouse;

            // Always allow ESCAPE key to go through to states for menu navigation
            bool is_escape_key = (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE);

            if (is_escape_key)
            {
                SDL_Log("ESCAPE key pressed - forwarding to state stack\n");
                // ESCAPE always goes through for menu navigation
                m_crafting_states->handle_event(event);
                continue; // Skip other event logic
            }

            // For other events, check if ImGui wants to capture them
            // Pass event to states if:
            // - It's not a keyboard/mouse event
            // - ImGui doesn't want to capture it
            bool should_forward_event =
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

    void render(double& current_time_step, const double elapsed) const noexcept
    {
        // Clear the screen before drawing
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        m_crafting_states->draw();

        handle_FPS(current_time_step, elapsed);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(this->m_sdl.window);
    }
}; // craft_impl

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

    static constexpr auto USE_CACHE = true;
    static constexpr auto COMMIT_INTERVAL = 5000;
    if (USE_CACHE)
    {
        db_enable();

        static constexpr auto DB_FILE = "craft.db";
        if (db_init(const_cast<char*>(DB_FILE)) != 0)
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

    this->m_impl->m_crafting_states->update(0.0f, rng);

    // BEGIN EVENT LOOP
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!this->m_impl->m_crafting_states->is_empty())
#endif
    {
        // FRAME RATE
        static constexpr auto FIXED_TIME_STEP = 1000.0 / 60.0;
        const auto current = SDL_GetTicks();
        const auto elapsed = static_cast<double>(current - previous);
        previous = current;
        accumulator += elapsed;

        // FLUSH DATABASE
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

            this->m_impl->update(FIXED_TIME_STEP, rng);
        }

        this->m_impl->render(time_step, elapsed);
    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    this->m_impl->m_sdl.destroy_and_quit();

    // db_save_state(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);
    db_close();
    db_disable();

    return true;
} // run

/**
 *
 *
 * @brief Used by Emscripten mostly to produce a JSON string containing the vertex data
 * @return returns JSON-encoded string: "{\"name\":\"MyMaze\", \"data\":\"v 1.0 1.0 0.0\\nv -1.0 1.0 0.0\\n...\"}";
 */
std::string craft::mazes() const noexcept
{
    return "";
}

/**
 * @brief Useful on mobile devices to flip mouse/finger capture
 *
 */
void craft::toggle_mouse() const noexcept
{
}
