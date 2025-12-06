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

#include <noise/noise.h>

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

#include <functional>
#include <memory>
#include <map>

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
        typedef std::unique_ptr<state> ptr;

        struct context
        {
            explicit context(SDL_Window *window, font_manager &fonts, texture_manager &textures, player &p)
                : m_window{window}, m_fonts{&fonts}, m_textures{&textures}, m_player{&p}
            {
            }

            SDL_Window *m_window;
            font_manager *m_fonts;
            texture_manager *m_textures;
            player *m_player;
        };

        explicit state(state_stack &stack, context context) : m_stack{&stack}, m_context{context}
        {
        }

        virtual void draw() const noexcept = 0;
        virtual bool update(float dt, unsigned int sub_steps, mazes::randomizer &rng) noexcept = 0;
        virtual bool handle_event(SDL_Event &event) noexcept = 0;

    protected:
        void request_stack_push(StateIdentifier state_id)
        {
            m_stack->push_state(state_id);
        }

        void request_stack_pop()
        {
            m_stack->pop_state();
        }

        void request_stack_clear()
        {
            m_stack->clear_states();
        }

        context get_context() const noexcept
        {
            return m_context;
        }

        state_stack &get_stack() const noexcept
        {
            return *m_stack;
        }

    private:
        state_stack *m_stack;
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
        explicit state_stack(state::context _context)
            : m_stack(), m_pending_list(), m_context(_context), m_factories()
        {
        }

        template <typename T>
        void register_state(StateIdentifier state_id)
        {
            m_factories.insert_or_assign(state_id, [this]()
                                         { return state::ptr(std::make_unique<T>(*this, m_context)); });
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

        void update(float dt, unsigned int sub_steps, mazes::randomizer &rng) noexcept
        {
            for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
            {
                if (!(*it)->update(dt, sub_steps, std::ref(rng)))
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

        void handle_event(SDL_Event &event) noexcept
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

        bool is_empty() const noexcept
        {
            return m_stack.empty();
        }

    private:
        state::ptr create_state(StateIdentifier state_id)
        {
            if (auto found = m_factories.find(state_id); found != m_factories.cend())
            {
                return found->second();
            }

            throw std::runtime_error("StateStack::createState - No factory found for state ID");
        }

        void apply_pending_changes()
        {
            for (const pending_change &change : m_pending_list)
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
    }; // state_stack

    // Handles main gameplay workflow (building, editing, and rendering the voxel world)
    class editor_state : public state
    {
    private:
        player *m_player;
        world m_world;
        float m_mouse_movement;

    public:
        explicit editor_state(state_stack &stack, context _context)
            : state{stack, _context}, m_world{_context.m_window, *_context.m_fonts, *_context.m_textures}, m_player{_context.m_player}
        {
        }

        void draw() const noexcept override
        {
            const auto &window = *get_context().m_window;

            //    mWorld.draw();
        }

        bool update(float dt, unsigned int sub_steps, mazes::randomizer &rng) noexcept override
        {
            // mWorld.update(dt);

            // auto& commands = mWorld.getCommandQueue();
            // m_player.handle_realtime_input(std::ref(commands));

            m_mouse_movement = SDL_min(0.0025, dt);

            return true;
        }

        bool handle_event(SDL_Event &event) noexcept override
        {
            // auto& commands = mWorld.getCommandQueue();

            // m_player.handle_event(event, std::ref(commands));
            // mWorld.handle_event(event);

            static float dy = 0;
            player::state *s = &m_player->s1;
            int sz = 0;
            int sx = 0;
            constexpr float directional_movement = 0.025f;
            auto current_scancode = SDL_SCANCODE_UNKNOWN;

            SDL_Keymod mod_state = SDL_GetModState();

            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    request_stack_clear();
                    break;
                case SDL_EVENT_KEY_DOWN:
                {
                    current_scancode = event.key.scancode;
                    switch (current_scancode)
                    {
                    case SDL_SCANCODE_ESCAPE:
                    {
                        SDL_SetWindowRelativeMouseMode(get_context().m_window, false);

                        request_stack_push(StateIdentifier::MENU);
                        break;
                    }

                    case SDL_SCANCODE_TAB:
                    {
                        // this->m_model->flying = !this->m_model->flying;
                        break;
                    }
                    break;
                    }
                    break;
                }
                } // switch
            } // SDL_Event

            return true;
        } // handle_events_and_motion
    };

    class loading_state : public state
    {

        void load_resources() const noexcept
        {
            static constexpr auto FONT_PIXEL_SIZE = 28.f;

            auto &&fonts = get_context().m_fonts;

            fonts->load(FontIdentifier::COUSINE_REGULAR, Cousine_Regular_compressed_data, Cousine_Regular_compressed_size, FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::LIMELIGHT, Limelight_Regular_compressed_data, Limelight_Regular_compressed_size, FONT_PIXEL_SIZE);
            fonts->load(FontIdentifier::NUNITO_SANS, NunitoSans_compressed_data, NunitoSans_compressed_size, FONT_PIXEL_SIZE);

            constexpr std::string_view atlas_path = "textures/atlas.png";
            constexpr std::string_view bitmap_font_path = "textures/bitmap_font.png";
            constexpr std::string_view window_icon_path = "textures/icon.bmp";
            constexpr std::string_view signs_path = "textures/signs.png";

            auto &&textures = get_context().m_textures;

            textures->load(TextureIdentifier::ATLAS, atlas_path, 0);
            textures->load(TextureIdentifier::BITMAP_FONT, bitmap_font_path, 1);
            textures->load(get_context().m_window, TextureIdentifier::WINDOW_ICON, window_icon_path);
            textures->load(TextureIdentifier::SIGNS, signs_path, 2);
        }

        void set_completion(float percent) noexcept
        {
        }

        bool m_has_finished;

    public:
        explicit loading_state(state_stack &_stack, state::context _context)
            : state(_stack, _context), m_has_finished{false}
        {
        }

        void draw() const noexcept override
        {
            const auto &window = *get_context().m_window;

            // window.draw(mLoadingSprite);
        }

        bool update(float dt, unsigned int sub_steps, mazes::randomizer &rng) noexcept override
        {
            if (!m_has_finished)
            {
                load_resources();
                m_has_finished = true;
            }

            return true;
        }

        bool handle_event(SDL_Event &event) noexcept override
        {
            return true;
        }

        bool is_finished() const noexcept
        {
            return m_has_finished;
        }
    };

    // Handles GUI options
    class menu_state : public state
    {
    public:
        explicit menu_state(state_stack &stack, context context)
            : state{stack, context}
        {
        }

        void draw() const noexcept override
        {
            const auto &window = *get_context().m_window;

            // window.draw(mLoadingSprite);
        }

        bool update(float dt, unsigned int sub_steps, mazes::randomizer &rng) noexcept override
        {
            return true;
        }

        bool handle_event(SDL_Event &event) noexcept override
        {
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                }
            }

            return true;
        }
    }; // menu_state

    const std::string &INIT_WINDOW_TITLE;
    const int INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT;

    std::unique_ptr<state_stack> m_crafting_states;

    font_manager m_fonts;
    texture_manager m_textures;

    player m_player;

    sdl_helper m_sdl;

    mutable double fps_update_timer = 0.0;
    mutable int smoothed_fps = 0;
    mutable float smoothed_frame_time = 0.0f;

    craft_impl(const std::string &title, int w, int h)
        : INIT_WINDOW_TITLE(title), INIT_WINDOW_WIDTH(w), INIT_WINDOW_HEIGHT(h)
    {
        start_SDL();

        setup_imgui();

        m_crafting_states = std::make_unique<state_stack>(state::context{m_sdl.window, std::ref(m_fonts), std::ref(m_textures), std::ref(m_player)});

        register_states();

        m_crafting_states->push_state(StateIdentifier::LOADING);
        m_crafting_states->push_state(StateIdentifier::EDITOR);
    }

    void register_states() noexcept
    {
        m_crafting_states->register_state<editor_state>(StateIdentifier::EDITOR);
        m_crafting_states->register_state<loading_state>(StateIdentifier::LOADING);
        m_crafting_states->register_state<menu_state>(StateIdentifier::MENU);
    }

    void start_SDL() noexcept
    {
        if (m_sdl.initialize(INIT_WINDOW_TITLE.c_str(), INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT))
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL initialized successfully.\n");
        }
    }

    void setup_imgui() noexcept
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
        std::string glsl_version = "";
#if defined(__EMSCRIPTEN__)
        glsl_version = "#version 100";
#else
        glsl_version = "#version 130";
#endif
        ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    }

    void handle_FPS(double &currentTimeStep, const double elapsed) const noexcept
    {
        constexpr double FPS_UPDATE_INTERVAL = 250.0;
        // Calculate instantaneous FPS and frame time
        const auto fps = static_cast<int>(1000.0 / elapsed);
        const auto frame_time = static_cast<float>(elapsed);

        // Update smoothed values periodically for display
        fps_update_timer += elapsed;
        if (fps_update_timer >= FPS_UPDATE_INTERVAL)
        {
            smoothed_fps = fps;
            smoothed_frame_time = frame_time;
            fps_update_timer = 0.0;
        }

        if (currentTimeStep >= 1000.0)
        {
            SDL_Log("FPS: %d\n", smoothed_fps);
            SDL_Log("Frame Time: %.3f ms/frame\n", smoothed_frame_time);

            currentTimeStep = 0.0;
        }

        // Create ImGui overlay window
        // Set window position to top-right corner
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

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

    void process_input() noexcept
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

            m_crafting_states->handle_event(event);
        }
    }

    void update(float dt, double time_step, mazes::randomizer &rng) noexcept
    {
        m_crafting_states->update(dt, time_step, std::ref(rng));
    }

    void render(double &current_time_step, const double elapsed) const noexcept
    {
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

craft::craft(const std::string &title, const int w, const int h)
    : m_impl{std::make_unique<craft_impl>(cref(title), w, h)}
{
}

craft::~craft() = default;

/**
 * Run the craft-engine in a loop with SDL window open
 */
bool craft::run([[maybe_unused]] mazes::grid_interface *g, mazes::randomizer &rng) const noexcept
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
        if (db_init(const_cast<char *>(DB_FILE)) != 0)
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

    this->m_impl->m_crafting_states->update(0.0f, 0, rng);

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

            this->m_impl->update(static_cast<float>(FIXED_TIME_STEP), time_step, rng);
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
