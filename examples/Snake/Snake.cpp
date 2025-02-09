/**
 * @brief Snake class implementation
 *  Simple 2D maze Snake using SDL3
 *  Press 'B' to generate a new maze
 * 
 * Threading technique uses 'islands':
 *  Example: https://github.com/SFML/SFML/tree/2.6.1/examples/island
 *
 * Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
 *
 */

#include "Snake.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <deque>
#include <sstream>
#include <iostream>
#include <random>
#include <functional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/maze_builder.h>

struct Snake::SnakeImpl {

    enum class States {
        // Snake is starting, show welcome screen
        SPLASH,
        // Main menu / configurations
        OPTIONS,
        // Snake is running
        PLAY,
        // Level is generated but Snake is paused/options
        PAUSE,
        // Snake is exiting and done
        DONE,
        // Useful when knowing when to re-draw in Snake loop
        // Level is being generated and not yet playable
        UPLOADING_LEVEL,
    };

    struct WorkItem {
        const std::vector<std::shared_ptr<mazes::cell>>& cells;
        const SDL_FPoint cellSize;
        std::vector<SDL_Vertex>& vertices;
        int start, count;

        WorkItem(const std::vector<std::shared_ptr<mazes::cell>>& cells, const SDL_FPoint cellSize,
            std::vector<SDL_Vertex>& vertices, int start, int count)
            : cells(cells), cellSize(cellSize)
            , vertices(vertices), start{ start }, count{ count } {
        }
    };

    // Wrapper for SDL_Texture
    struct SDLTexture {
    private:
        SDL_Texture* texture;
        int width, height;
    public:
        SDLTexture() : texture{ nullptr }, width(0), height(0) {
        }

        ~SDLTexture() {
            this->free();
        }

        void free() noexcept {
            if (this->texture) {
                SDL_DestroyTexture(texture);
            }
        }

        SDL_Texture* get() const noexcept {
            return this->texture;
        }

        bool loadTarget(SDL_Renderer* renderer, int w, int h) {
            this->free();
            this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
            if (!this->texture) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture: %s\n", SDL_GetError());
            } else {
                this->width = w;
                this->height = h;
            }
            return this->texture != nullptr;
        }

        void render(SDL_Renderer *renderer, int x, int y) const noexcept {
            SDL_FRect renderQuad = { static_cast<float>(x),
                static_cast<float>(y),
                static_cast<float>(width),
                static_cast<float>(height) };
            SDL_RenderTexture(renderer, texture, nullptr, &renderQuad);
        }
    };

    struct SDLHelper {
        SDL_Window* window;
        SDL_Renderer* renderer;

        SDL_AudioDeviceID audioDeviceId;
        SDL_AudioStream* audioStream;
        std::uint8_t* wavBuffer;
        std::uint32_t wavLength;
        SDL_AudioSpec audioSpec;

        /**
         * @brief Init SDL subsystems - use iostream on error messages
         *  since the SDL_Log functions are not available in this context
         *
         */
        SDLHelper()
            : window{ nullptr }, renderer{ nullptr }
            , audioDeviceId{}, audioStream{}
            , wavBuffer{}, wavLength{}, audioSpec{} {
            using namespace std;
            
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
                SDL_Log("SDL_Init success\n");
            } else {
                cerr << "SDL_Init Error: " << endl;
            }
        }

        ~SDLHelper() {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);

            // Audio device only cleaned if it is played/binded
            if (this->audioDeviceId) {
                SDL_free(this->wavBuffer);
                SDL_DestroyAudioStream(this->audioStream);
                SDL_CloseAudioDevice(this->audioDeviceId);
            }

            SDL_Quit();
        }

        bool loadFont(const std::string& f, unsigned int fSize) noexcept {
            return false;
        }

        void do_events(States& state) noexcept {
            using namespace std;
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_KEY_DOWN) {
                    if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
                        state = States::DONE;
                        break;
                    } else if (e.key.scancode == SDL_SCANCODE_B) {
                        state = States::UPLOADING_LEVEL;
                    }
                } else if (e.type == SDL_EVENT_QUIT) {
                    state = States::DONE;
                    break;
                }
            }

        }

        void playAudioStream() noexcept {
            if (this->audioDeviceId) {
                SDL_BindAudioStream(this->audioDeviceId, this->audioStream);
                SDL_ResumeAudioStreamDevice(this->audioStream);
            }
        }

        void pauseAudioStream() noexcept {
            if (this->audioDeviceId) {
                SDL_PauseAudioStreamDevice(this->audioStream);
            }
        }

        void stopAudioStream() noexcept {
            if (this->audioDeviceId) {
                SDL_UnbindAudioStream(this->audioStream);
                SDL_FlushAudioStream(this->audioStream);
            }
        }
    
        bool loadWAV(const std::string& path) noexcept {
            if (!SDL_LoadWAV(path.c_str(), &audioSpec, &wavBuffer, &wavLength)) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load WAV file: %s\n%s\n", path.c_str(), SDL_GetError());
                return false;
            }
            return true;
        }
    };

    const std::string& title;
    const std::string& version;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    SDLHelper sdlHelper;
    // Thread helpers
    std::deque<WorkItem> workQueue;
    std::vector<SDL_Thread*> threads;
    SDL_Mutex* gameMtx;
    SDL_Condition* gameCond;

    SDLTexture entityTexture;

    // Global - Keep track of worker work count
    int pendingWorkCount;
    // Keep track of user and Snake states
    States state;

    SnakeImpl(const std::string& title, const std::string& version, int w, int h)
        : title{ title }, version{ version }, INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , sdlHelper{}, workQueue{}
        , gameMtx{ nullptr }, gameCond{ nullptr }
        , pendingWorkCount{ 0 }, state{ States::SPLASH } {
        gameCond = SDL_CreateCondition();
        if (!gameCond) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating condition variable: %s\n", SDL_GetError());
        }
        gameMtx = SDL_CreateMutex();
        if (!gameMtx) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating mutex: %s\n", SDL_GetError());
        }
        this->initWorkers();
    }

    ~SnakeImpl() {

    }

    /**
     * @brief C-style function for SDL_Thread
     *  Handles when workers should process work items
     *
     */
    static int threadFunc(void* data) {
        using namespace std;
        auto* Snake = reinterpret_cast<SnakeImpl*>(data);
        vector<SDL_Vertex> vertices;

        while (1) {
            {
                SDL_LockMutex(Snake->gameMtx);
                while (Snake->workQueue.empty() && Snake->state != States::DONE) {
                    SDL_WaitCondition(Snake->gameCond, Snake->gameMtx);
                }

                if (Snake->state == States::DONE) {
                    SDL_UnlockMutex(Snake->gameMtx);
                    break;
                }

                // Now process a work item
                if (!Snake->workQueue.empty()) {
                    auto&& temp = Snake->workQueue.front();
                    Snake->workQueue.pop_front();
                    SDL_Log("Processing work item [ start: %d | count: %d]\n", temp.start, temp.count);
                    Snake->doWork(ref(vertices), cref(temp));
                    if (!vertices.empty()) {
                        copy(vertices.begin(), vertices.end(), back_inserter(temp.vertices));
                    } else {
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "No vertices generated\n");
                    }
                }
                SDL_UnlockMutex(Snake->gameMtx);
            }

            {
                // Update work count and wake up any threads waiting
                SDL_LockMutex(Snake->gameMtx);
                Snake->pendingWorkCount -= 1;
                SDL_Log("Pending work count: %d\n", Snake->pendingWorkCount);
                if (Snake->pendingWorkCount <= 0) {
                    SDL_SignalCondition(Snake->gameCond);
                }
                SDL_UnlockMutex(Snake->gameMtx);
            }
        }

        return 0;
    }

    /**
     * @brief Trigger work queue changes and signal threads
     *  Compute target block space for workers to process
     *  Construct all work items
     */
    void genLevel(std::vector<SDL_Vertex>& vertices, const std::vector<std::shared_ptr<mazes::cell>>& cells, SDL_FPoint cellSize) noexcept {
        using namespace std;

        //this->state = States::UPLOADING_LEVEL;
        if (this->pendingWorkCount == 0) {
            SDL_WaitCondition(gameCond, gameMtx);
        }

        // Each worker processes a block of vertices
        static constexpr auto BLOCK_COUNT = 4;
        static auto VERTS_PER_BLOCK = cells.size() / BLOCK_COUNT;
        for (auto w{ 0 }; w < BLOCK_COUNT; w++) {
            auto pointStart = w * VERTS_PER_BLOCK;
            auto pointCount = (w == BLOCK_COUNT - 1) ? cells.size() - pointStart : VERTS_PER_BLOCK;
            workQueue.push_back({ cref(cells), cellSize, ref(vertices), static_cast<int>(pointStart), static_cast<int>(pointCount) });
        }
        SDL_LockMutex(this->gameMtx);
        this->pendingWorkCount = BLOCK_COUNT;
        SDL_SignalCondition(gameCond);
        SDL_UnlockMutex(this->gameMtx);
    }

private:
    /**
     * @brief Create a thread and a null work item for each worker
     *  Init the work item when gen_maze is called
     *
     */
    void initWorkers() noexcept {
        using namespace std;

        static constexpr auto NUM_WORKERS = 4;
        for (auto w{ 0 }; w < NUM_WORKERS; w++) {
            string name = { "thread: " + to_string(w) };
            SDL_Thread* temp = SDL_CreateThread(threadFunc, name.data(), this);
            if (!temp) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateThread failed: %s\n", SDL_GetError());
            }
            threads.push_back(temp);
        }
    }

    /**
     * @brief Perform rendering operations on the work item
     *  Design maze structure using SDL_FPoint data
     *  Interprating the maze as having cells with possible 1-3 walls per cell
     *  Each work item has a const-reference to the cells
     *  The cells are initialized @genLevel
     *
     * @param vertices
     * @param item the work item to process
     *
     */
    void doWork(std::vector<SDL_Vertex>& vertices, const WorkItem& item) {
        using namespace std;

        auto pushV = [&vertices](auto v1, auto v2, auto v3, auto v4)->void {
            // First triangle
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v4);

            // Second triangle
            vertices.push_back(v2);
            vertices.push_back(v3);
            vertices.push_back(v4);
            };

        const auto& cells = item.cells;

        uint32_t wallColor = 0x000000FF;
        SDL_FColor sdlWallColor = { static_cast<float>((wallColor >> 24) & 0xFF),
            static_cast<float>((wallColor >> 16) & 0xFF),
            static_cast<float>((wallColor >> 8) & 0xFF),
            static_cast<float>(wallColor & 0xFF) };


        // @TEST ! This is a sample triangle
        //// Vertex 1
        //SDL_Vertex v1, v2, v3;
        //v1.position = { 400.0f, 100.0f };
        //v1.color = { 255, 0, 0, 255 };
        //v1.tex_coord = { 0.0f, 0.0f };

        //// Vertex 2
        //v2.position = { 700.0f, 500.0f };
        //v2.color = { 0, 255, 0, 255 };
        //v2.tex_coord = { 1.0f, 1.0f };

        //// Vertex 3
        //v3.position = { 100.0f, 500.0f };
        //v3.color = { 0, 0, 255, 255 };
        //v3.tex_coord = { 0.0f, 1.0f };

        //vertices.push_back(v1);
        //vertices.push_back(v2);
        //vertices.push_back(v3);

        for (const auto& mode : { "backgrounds", "walls" }) {
            // Iterate only over the cells that are in the work item
            //  Note the '<' operator to ensure derefencing the "end" iterator is not done
            for (auto itr = cells.cbegin() + item.start; itr < cells.cbegin() + item.start + item.count; ++itr) {
                auto&& current = *itr;

                const auto& cellSize = item.cellSize;

                auto x1 = static_cast<float>(current->get_column()) * cellSize.x;
                auto y1 = static_cast<float>(current->get_row()) * cellSize.y;
                auto x2 = static_cast<float>(current->get_column() + 1) * cellSize.x;
                auto y2 = static_cast<float>(current->get_row() + 1) * cellSize.y;

                SDL_Vertex v1, v2, v3, v4;
                // Define the four corners of the cell
                v1.position = { static_cast<float>(x1), static_cast<float>(y1) };
                v2.position = { static_cast<float>(x2), static_cast<float>(y1) };
                v3.position = { static_cast<float>(x2), static_cast<float>(y2) };
                v4.position = { static_cast<float>(x1), static_cast<float>(y2) };
                v1.tex_coord = { 0.0f, 0.0f };
                v2.tex_coord = { 1.0f, 0.0f };
                v3.tex_coord = { 1.0f, 1.0f };
                v4.tex_coord = { 0.0f, 1.0f };

                if (mode == "backgrounds"s) {
                    uint32_t color = 0xFFFFFFFF;
                    SDL_FColor sdlColor = { static_cast<float>((color >> 24) & 0xFF),
                        static_cast<float>((color >> 16) & 0xFF),
                        static_cast<float>((color >> 8) & 0xFF),
                        static_cast<float>(color & 0xFF) };

                    v1.color = v2.color = v3.color = v4.color = sdlColor;

                    pushV(v1, v2, v3, v4);
                } else {
                    v1.color = v2.color = v3.color = v4.color = sdlWallColor;

                    if (!current->get_north()) {
                        pushV(v1, v2, v3, v4);
                    }
                    if (!current->get_west()) {
                        pushV(v1, v2, v3, v4);
                    }
                    if (auto east = current->get_east(); east && !current->is_linked(east)) {
                        pushV(v1, v2, v3, v4);
                    }
                    if (auto south = current->get_south(); south && !current->is_linked(south)) {
                        pushV(v1, v2, v3, v4);
                    }
                }
            } // cells
        } // background / walls
    } // doWork
}; // SnakeImpl


Snake::Snake(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<SnakeImpl>(std::cref(title), std::cref(version), w, h)} {
}

Snake::~Snake() {
    auto&& g = this->m_impl;
    // Clean up threads
    SDL_LockMutex(g->gameMtx);
    // Wake up any threads and wait for them to finish
    g->pendingWorkCount = 0;
    g->state = SnakeImpl::States::DONE;
    SDL_BroadcastCondition(g->gameCond);
    SDL_UnlockMutex(g->gameMtx);
    for (auto&& t : g->threads) {
        auto name = SDL_GetThreadName(t);
        int status = 0;
        SDL_WaitThread(t, &status);
        SDL_Log("Worker thread with status [ %s | %d ] to finish\n", name, status);
    }
    SDL_DestroyMutex(g->gameMtx);
    SDL_DestroyCondition(g->gameCond);
}

bool Snake::run() const noexcept {

    using namespace std;

    // Create an alias to the Snake implementation
    auto&& sdlHelper = this->m_impl->sdlHelper;
    string_view titleView = this->m_impl->title;
    sdlHelper.window = SDL_CreateWindow(titleView.data(), this->m_impl->INIT_WINDOW_W, this->m_impl->INIT_WINDOW_H, SDL_WINDOW_RESIZABLE);
    sdlHelper.renderer = SDL_CreateRenderer(sdlHelper.window, nullptr);

    SDL_Surface* icon = SDL_LoadBMP("resources/icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(sdlHelper.window, icon);
        SDL_DestroySurface(icon);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s\n", SDL_GetError());
    }

    // Setup SDL audio device
    static const string loadingWAV = "resources/loading.wav";
    if (!sdlHelper.loadWAV(loadingWAV)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load WAV file\n");
    }
    sdlHelper.audioStream = SDL_CreateAudioStream(&sdlHelper.audioSpec, &sdlHelper.audioSpec);
    if (!sdlHelper.audioStream) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create audio stream: %s\n", SDL_GetError());
    }
    sdlHelper.audioDeviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!sdlHelper.audioDeviceId) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open audio device: %s\n", SDL_GetError());
    }
    SDL_PutAudioStreamData(sdlHelper.audioStream, sdlHelper.wavBuffer, sdlHelper.wavLength);
    SDL_FlushAudioStream(sdlHelper.audioStream);
    static auto stream_bytes_len = SDL_GetAudioStreamAvailable(sdlHelper.audioStream);

    sdlHelper.playAudioStream();

    auto&& renderer = sdlHelper.renderer;
    SDL_SetRenderVSync(renderer, true);
    auto&& window = sdlHelper.window;

    Snake::SnakeImpl::SDLTexture renderToTexture;
    bool res = renderToTexture.loadTarget(renderer, this->m_impl->INIT_WINDOW_W, this->m_impl->INIT_WINDOW_H);
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load target texture: %s\n", SDL_GetError());
    }

    // Define the level by its vertices renderizable
    vector<SDL_Vertex> level;
    vector<shared_ptr<mazes::cell>> cells;

    // Timers for keeping track of frame rates
    double previous = SDL_GetTicks();
    double accumulator = 0.0, currentTimeStep = 0.0;
    // Snake loop
    auto&& gState = this->m_impl->state;
    while (gState != SnakeImpl::States::DONE) {
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = SDL_GetTicks() - previous;
        previous = SDL_GetTicks();

        accumulator += elapsed;
        while (accumulator >= FIXED_TIME_STEP) {
            // Input events
            sdlHelper.do_events(ref(gState));

            // Update
            accumulator -= elapsed;
            currentTimeStep += FIXED_TIME_STEP;
        }

        // Update screen-related properties
        int display_w, display_h;
        SDL_GetWindowSize(window, &display_w, &display_h);
        float aspectRatioMin, aspectRatioMax;
        SDL_GetWindowAspectRatio(window, &aspectRatioMin, &aspectRatioMax);

        SDL_FPoint screenCenter = { static_cast<float>(display_w / 2), static_cast<float>(display_h / 2) };

        // Update FPS data - Check if it's been about a second
        if (currentTimeStep >= 1.0) {
            SDL_Log("FPS: %d\n", static_cast<int>(currentTimeStep * 60.0));
            SDL_Log("Frame Time / Update: %.3fms\n", elapsed / currentTimeStep);
            currentTimeStep = 0.0;
        }

        // Audio stream updates
        if (SDL_GetAudioStreamAvailable(sdlHelper.audioStream) < sdlHelper.wavLength) {
            SDL_PutAudioStreamData(sdlHelper.audioStream, sdlHelper.wavBuffer, sdlHelper.wavLength);
            stream_bytes_len = SDL_GetAudioStreamAvailable(sdlHelper.audioStream);
        }

        SDL_SetRenderTarget(renderer, renderToTexture.get());

        // Render prep
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw/gen the level
        if (!this->m_impl->pendingWorkCount) {
            if (gState == SnakeImpl::States::UPLOADING_LEVEL) {
                // Update state and current level
                SDL_Log("New level uploading\n");
                // mazes::builder builder;
                // static constexpr auto INIT_MAZE_ROWS = 100, INIT_MAZE_COLS = 50;
                // auto&& temp = builder.block_type(-1).rows(INIT_MAZE_ROWS).columns(INIT_MAZE_COLS).build();
                // temp->init();
                // mazes::computations::compute_geometry(temp);
                // cells.clear();
                // temp->populate_cells(ref(cells));
                // assert(cells.size() > 0);
                // level.clear();
                // level.reserve(cells.size());
                auto rows = 10, columns = 10;
                SDL_FPoint cellSize = { static_cast<float>(display_w / rows), static_cast<float>(display_h / columns) };

                // Now start the worker threads
                this->m_impl->genLevel(ref(level), cref(cells), cellSize);

                gState = SnakeImpl::States::PLAY;
            }

            // Now draw geometry ensuring complete render with no more work pending
            SDL_RenderGeometry(renderer, nullptr, level.data(), level.size(), nullptr, 0);
        }

        // Finally, draw text to screen
        SDL_SetRenderDrawColor(renderer, 255, 0, 175, 255);

        SDL_SetRenderTarget(renderer, nullptr);

        SDL_RenderTexture(renderer, renderToTexture.get(), nullptr, nullptr);

        SDL_RenderPresent(renderer);
    }

    return true;
} // run
