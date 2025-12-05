// Showcases basic voxel world and editing capabilities
// Follows game development patterns like Command Queues and State Stacks
// Creates mazes with Maze Builder library
// Originally written in C99, and I ported it to C++17

#include "craft.h"

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>
#include "nunito_sans.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten_local/emscripten_mainloop_stub.h>
#else
#endif

#include <SDL3/SDL.h>

#include <algorithm>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <utility>
#include <thread>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>

#include <noise/noise.h>

#include "craft_utils.h"
#include "cube.h"
#include "db.h"
#include "world.h"
#include "sign.h"
#include "item.h"
#include "map.h"
#include "matrix.h"
#include "sdl_helper.h"

#include "gl_resource_manager.h"
#include "bloom_effects.h"
#include "stencil_renderer.h"
#include "maze_projector.h"

#include <MazeBuilder/maze_builder.h>

// Namespace alias for convenience
namespace cr = craft_rendering;

// Movement configurations
#define KEY_FORWARD SDL_SCANCODE_W
#define KEY_BACKWARD SDL_SCANCODE_S
#define KEY_LEFT SDL_SCANCODE_A
#define KEY_RIGHT SDL_SCANCODE_D
#define KEY_JUMP SDL_SCANCODE_SPACE
#define KEY_FLY SDL_SCANCODE_TAB
#define KEY_ITEM_NEXT SDL_SCANCODE_E
#define KEY_ITEM_PREV SDL_SCANCODE_R
#define KEY_ZOOM SDL_SCANCODE_LSHIFT
#define KEY_ORTHO SDL_SCANCODE_F
#define KEY_TAG SDL_SCANCODE_T

// World configs
#define SCROLL_THRESHOLD 0.1
#define MAX_DB_PATH_LEN 64
#define USE_CACHE true
#define DAY_LENGTH 600
#define INVERT_MOUSE 0
#define MAX_TEXT_LENGTH 256

// Advanced options
#define COMMIT_INTERVAL 7
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 20
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define MAX_CHUNKS 8192
#define MAX_PLAYERS 1
#define NUM_WORKERS 4

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

namespace
{
    enum class Entity : unsigned int
    {
        NONE = 0,
        SCENE = 1 << 0,
        PLAYER = 1 << 1,
        ENEMY = 1 << 2,
        PROJECTILE = 1 << 3,
        PICKUP = 1 << 4,
        ALL = 1 << 5
    };

    enum class MenuItem : unsigned int
    {
        CONTINUE = 0,
        NEW_GAME = 1,
        SETTINGS = 2,
        SPLASH = 3,
        QUIT = 4,
        COUNT = 5
    };

    enum class PlayerAction
    {
        MOVE_LEFT,
        MOVE_RIGHT,
        JUMP,
        COUNT
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

struct craft::craft_impl
{

    typedef struct
    {
        Map map;
        Map lights;
        SignList signs;
        int p;
        int q;
        int faces;
        int sign_faces;
        int dirty;
        int miny;
        int maxy;
        GLuint buffer;
        GLuint sign_buffer;
    } Chunk;

    struct WorkerItem
    {
        int p;
        int q;
        int load;
        Map *block_maps[3][3];
        Map *light_maps[3][3];
        int miny;
        int maxy;
        int faces;
        GLfloat *data;
        WorkerItem()
        {
        }
    };

    typedef struct
    {
        int index;
        int state;
        std::thread thrd;
        std::mutex mtx;
        std::condition_variable cnd;
        WorkerItem item;
        bool should_stop;
    } Worker;

    typedef struct
    {
        int x;
        int y;
        int z;
        int w;
    } Block;

    typedef struct
    {
        float x;
        float y;
        float z;
        float rx;
        float ry;
        float t;
    } State;

    typedef struct
    {
        GLuint program;
        GLuint position;
        GLuint normal;
        GLuint uv;
        GLuint matrix;
        GLuint sampler;
        GLuint camera;
        GLuint timer;
        GLuint extra1;
        GLuint extra2;
        GLuint extra3;
        GLuint extra4;
    } Attrib;

    typedef struct
    {
        std::vector<std::unique_ptr<Worker>> workers;
        Chunk chunks[MAX_CHUNKS];
        int chunk_count;
        int create_radius;
        int render_radius;
        int delete_radius;
        int sign_radius;
        int player_count;
        int voxel_scene_w;
        int voxel_scene_h;
        bool flying;
        int item_index;
        int scale;
        bool is_ortho;
        float fov;
        char db_path[MAX_DB_PATH_LEN];
        int day_length;
        int start_time;
        int start_ticks;
        Block block0;
        Block block1;
        Block copy0;
        Block copy1;
    } Model;

    class gui_options
    {
    public:
        bool fullscreen;
        bool vsync;
        bool color_mode_dark;
        bool capture_mouse;
        int chunk_size;
        bool show_items;
        bool show_wireframes;
        bool show_crosshairs;
        bool show_info_text;
        bool apply_bloom_effect;
        float exposure;
        char outfile[64];
        int seed;
        int rows;
        int height;
        int columns;
        int offset_x;
        int offset_z;
        std::string algo;
        int view;
        char tag[MAX_SIGN_LENGTH];

        void reset()
        {
            for (auto i = 0; i < IM_ARRAYSIZE(outfile); ++i)
            {
                outfile[i] = '\0';
            }
            outfile[0] = '.';
            outfile[1] = 'o';
            outfile[2] = 'b';
            outfile[3] = 'j';
            rows = 15;
            height = 5;
            columns = 28;
            view = 20;
            algo = "binary_tree";
            seed = 10;
            chunk_size = 8;
            tag[0] = 'H';
            tag[1] = 'i';
            show_crosshairs = true;
            show_info_text = true;
            show_items = true;
            show_wireframes = true;
            capture_mouse = false;
        }
    }; // class

    class scene_node;

    struct command
    {
        std::function<void(scene_node &, float)> action;
        Entity category;
    };

    template <typename GameObject, typename Function>
    std::function<void(scene_node &, float)> derived_action(Function fn)
    {
        return [=](scene_node &node, float dt)
        {
            // Ensure that the cast is safe
            if constexpr (std::is_base_of_v<GameObject, scene_node>)
            {
                fn(static_cast<GameObject &>(node), dt);
            }
        };
    }

    class command_queue
    {
    public:
        void push(const command &command)
        {
            commands.push(command);
        }

        command pop()
        {
            command cmd = commands.front();
            commands.pop();
            return cmd;
        }

        bool is_empty() const
        {
            return commands.empty();
        }

    private:
        std::queue<command> commands;
    };

    class player
    {
    public:
        std::map<std::uint32_t, PlayerAction> m_key_binding;

        std::map<PlayerAction, command> m_action_binding;

        bool m_is_active;

        std::string m_name;
        State m_state;
        State m_state1;
        State m_state2;
        GLuint m_buffer;

    public:
        explicit player() : m_is_active(true)
        {
            m_key_binding[SDL_SCANCODE_LEFT] = PlayerAction::MOVE_LEFT;
            m_key_binding[SDL_SCANCODE_RIGHT] = PlayerAction::MOVE_RIGHT;
            m_key_binding[SDL_SCANCODE_SPACE] = PlayerAction::JUMP;

            initialize_actions();

            for (auto &pair : m_action_binding)
            {
                pair.second.category = Entity::PLAYER;
            }
        }

        void handle_event(SDL_Event &event, command_queue &commands) noexcept
        {
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    auto found = m_key_binding.find(event.key.scancode);

                    if (found != m_key_binding.cend() && !is_realtime_action(found->second))
                    {
                        if (found->second == PlayerAction::JUMP)
                        {
                            return; // do not jump if not on ground
                        }
                        commands.push(m_action_binding[found->second]);
                    }
                }
                if (event.type == SDL_SCANCODE_RETURN)
                {
                }
            }
        }
        void handle_realtime_input(command_queue &commands)
        {
            for (auto &pair : m_key_binding)
            {
                if (is_realtime_action(pair.second))
                {
                    int numKeys = 0;
                    const auto *keyState = SDL_GetKeyboardState(&numKeys);

                    if (keyState && pair.first < static_cast<std::uint32_t>(numKeys) && keyState[pair.first])
                    {
                        commands.push(m_action_binding[pair.second]);
                    }
                }
            }
        }

        void assign_key(PlayerAction action, std::uint32_t key)
        {
            // Remove all keys that already map to action
            for (auto it = m_key_binding.begin(); it != m_key_binding.end();)
            {
                if (it->second == action)
                    it = m_key_binding.erase(it);
                else
                    ++it;
            }

            // Insert new binding
            m_key_binding[key] = action;
        }

        [[nodiscard]] std::uint32_t get_assigned_key(PlayerAction action) const
        {
            for (auto &pair : m_key_binding)
            {
                if (pair.second == action)
                    return pair.first;
            }

            return SDL_SCANCODE_UNKNOWN;
        }

        bool is_active() const noexcept
        {
            return m_is_active;
        }
        void set_active(bool active) noexcept
        {
            m_is_active = active;
        }

    private:
        void initialize_actions()
        {
            static constexpr auto playerSpeed = 200.f;
            static constexpr auto jumpForce = -500.f;

            // Note: derived_action is a member function of craft_impl,
            // so we'll use a simple lambda instead
            m_action_binding[PlayerAction::MOVE_LEFT].action = [](scene_node &node, float dt)
            {
                // Do something for move left action
            };

            // on create block

            // on destroy block

            // on copy block
        }

        static bool is_realtime_action(PlayerAction action)
        {
            switch (action)
            {
            case PlayerAction::MOVE_LEFT:
            case PlayerAction::MOVE_RIGHT:
                return true;
            default:
                return false;
            }
        }
    };

    class state_stack;

    class state
    {
    public:
        typedef std::unique_ptr<state> ptr;

        struct context
        {
            explicit context(SDL_Window *window /*, FontManager& fonts, TextureManager& textures*/, player &p)
                : window{window}, p{&p}
            {
            }

            SDL_Window *window;
            // FontManager* fonts;
            // TextureManager* textures;
            player *p;
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

        template <typename T, typename ResourcePath>
        void register_state(StateIdentifier state_id, ResourcePath &&resource_path)
        {
            m_factories.insert_or_assign(state_id, [this, resource_path = std::forward<ResourcePath>(resource_path)]()
                                         { return state::ptr(std::make_unique<T>(*this, m_context, resource_path)); });
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
        player m_player;
        // World mWorld;
        gui_options m_gui;
        float m_mouse_movement;

    public:
        explicit editor_state(state_stack &stack, context context)
            : state{stack, context}
              //   , mWorld{*context.window, *context.fonts, *context.textures}
              ,
              m_player{*context.p}
        {
        }

        void draw() const noexcept override
        {
            const auto &window = *get_context().window;

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
            State *s = &m_player.m_state;
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
                        SDL_SetWindowRelativeMouseMode(get_context().window, false);
                        this->m_gui.capture_mouse = false;
                        this->m_gui.fullscreen = false;
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
    private:
        void load_resources() noexcept
        {
        }

        void load_window_icon(const std::unordered_map<std::string, std::string> &resources) noexcept
        {
        }

        void set_completion(float percent) noexcept
        {
        }

        bool m_has_finished;

        std::string m_resource_path;

    public:
        explicit loading_state(state_stack &_stack, state::context _context, std::string_view resource_path = "resources")
            : state(_stack, _context), m_has_finished{false}, m_resource_path{resource_path}
        {

            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "LoadingState: resource path is: %s\n", m_resource_path.data());
            m_has_finished = true;
        }

        void draw() const noexcept override
        {
            const auto &window = *get_context().window;

            // window.draw(mLoadingSprite);
        }

        bool update(float dt, unsigned int sub_steps, mazes::randomizer &rng) noexcept override
        {
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
            const auto &window = *get_context().window;

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
                    // set flag

                    /*

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    destroy_and_quit();
                    */
                }
            }

            return true;
        }
    }; // menu_state

    const std::string &INIT_WINDOW_TITLE;
    const int INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT;

    std::unique_ptr<state_stack> m_crafting_states;

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

        m_crafting_states = std::make_unique<state_stack>(state::context{m_sdl.window, /*m_fonts, m_textures,*/ m_player});

        register_states();

        m_crafting_states->push_state(StateIdentifier::LOADING);
        m_crafting_states->push_state(StateIdentifier::EDITOR);
    }

    void register_states() noexcept
    {
        constexpr std::string_view resource_path = "resources";
        m_crafting_states->register_state<editor_state>(StateIdentifier::EDITOR);
        m_crafting_states->register_state<loading_state>(StateIdentifier::LOADING, resource_path);
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

    CHECK_GL_ERR();

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

        CHECK_GL_ERR();

    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    SDL_Log("Closing DB. . .\n");

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
