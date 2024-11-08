/**
 * @brief Game class implementation
 *  Design a game with a "secret theme" for GitHub Gamejam 2024
 *  This program allows hardware-accelerated graphics.
 *  Optional Http requests can be made to Cloudflare Workers
 *
 *  - "Secret theme" is how to solve maze puzzles
 *
 * Threading technique used to perform rendering tasks
 *  Example: https://github.com/SFML/SFML/tree/2.6.1/examples/island
 *
 *
 */

#include "Game.hpp"

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
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include <box2d/box2d.h>
#include <box2d/math_functions.h>

#include <MazeBuilder/maze_builder.h>

struct Game::GameImpl {

    enum class States {
        // Game is starting, show welcome screen
        SPLASH,
        // Main menu / configurations
        OPTIONS,
        // Game is running
        PLAY,
        // Level is generated but game is paused/options
        PAUSE,
        // Game is exiting and done
        DONE,
        // Useful when knowing when to re-draw in game loop
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

        bool loadfromImage(SDL_Renderer* renderer,const std::string& path) {
            this->free();
            SDL_Surface* surface = IMG_Load(path.c_str());
            if (surface) {
                this->texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (!this->texture) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from surface: %s\n", SDL_GetError());
                } else {
                    this->width = surface->w;
                    this->height = surface->h;
                }
                SDL_DestroySurface(surface);
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load image: %s\n", SDL_GetError());
            }
            return this->texture != nullptr;
        }

        bool loadFromRenderedText(SDL_Renderer *renderer, TTF_Font* font, const std::string& text, SDL_Color textColor) noexcept {
            this->free();
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), text.length(), textColor);
            if (textSurface) {
                this->texture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (this->texture) {
                    this->width = textSurface->w;
                    this->height = textSurface->h;
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from rendered text!");
                }
                SDL_DestroySurface(textSurface);
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create text surface!");
                return false;
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
        TTF_Font* font1;

        /**
         * @brief Init SDL subsystems - use iostream on error messages
         *  since the SDL_Log functions are not available in this context
         *
         */
        SDLHelper()
            : window{ nullptr }, renderer{ nullptr }, font1{ nullptr } {
            using namespace std;

            if (SDL_Init(SDL_INIT_VIDEO)) {
                SDL_Log("SDL_Init success\n");
            } else {
                cerr << "SDL_Init Error: " << endl;
            }

            if (TTF_Init()) {
                SDL_Log("TTF_Init success\n");
            } else {
                cerr << "TTF_Init Error: " << endl;
            }

            auto sdlImgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
            if (IMG_Init(sdlImgFlags) & sdlImgFlags) {
                SDL_Log("IMG_Init success\n");
            } else {
                cerr << "IMG_Init Error: " << endl;
            }
        }

        ~SDLHelper() {
            TTF_CloseFont(font1);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
        }

        bool loadFont(const std::string& f, unsigned int fSize) noexcept {
            font1 = TTF_OpenFont(f.c_str(), fSize);
            if (font1) {
                SDL_Log("Font loaded: %s\n", f.c_str());
                return true;
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Font not loaded: %s\n", f.c_str());
                return false;
            }
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
    };

    /**
     * @brief Entity class for game objects
     *  Has physics, texture, and interaction data
     *
     */
    struct Entity {
        b2BodyId bodyId;
        b2Vec2 extent;

        Entity() : bodyId{}, extent{ 0.0f, 0.0f } {
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

    // Physics/box2d world opaque ptr
    b2WorldId worldId;
    std::vector<std::unique_ptr<Entity>> entities;
    SDLTexture entityTexture;

    // Global - Keep track of worker work count
    int pendingWorkCount;
    // Keep track of user and game states
    States state;

    GameImpl(const std::string& title, const std::string& version, int w, int h)
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

    ~GameImpl() {

    }

    /**
     * @brief C-style function for SDL_Thread
     *  Handles when workers should process work items
     *
     */
    static int threadFunc(void* data) {
        using namespace std;
        auto* game = reinterpret_cast<GameImpl*>(data);
        vector<SDL_Vertex> vertices;

        while (1) {
            {
                SDL_LockMutex(game->gameMtx);
                while (game->workQueue.empty() && game->state != States::DONE) {
                    SDL_WaitCondition(game->gameCond, game->gameMtx);
                }

                if (game->state == States::DONE) {
                    SDL_UnlockMutex(game->gameMtx);
                    break;
                }

                // Now process a work item
                if (!game->workQueue.empty()) {
                    auto&& temp = game->workQueue.front();
                    game->workQueue.pop_front();
                    SDL_Log("Processing work item [ start: %d | count: %d]\n", temp.start, temp.count);
                    vertices.clear();
                    game->doWork(ref(vertices), cref(temp));
                    if (!vertices.empty()) {
                        copy(vertices.begin(), vertices.end(), back_inserter(temp.vertices));
                    } else {
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "No vertices generated\n");
                    }
                }
                SDL_UnlockMutex(game->gameMtx);
            }

            {
                // Update work count and wake up any threads waiting
                SDL_LockMutex(game->gameMtx);
                game->pendingWorkCount -= 1;
                SDL_Log("Pending work count: %d\n", game->pendingWorkCount);
                if (game->pendingWorkCount <= 0) {
                    SDL_SignalCondition(game->gameCond);
                }
                SDL_UnlockMutex(game->gameMtx);
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

    /**
     * @brief Create entities for the game
     *  Incorporate physics rigid body simulation and textures
     *
     */
    void initEntities() noexcept {
        // Init physics
        // Allow box2d to use pixel coordinates
        static constexpr auto FORCE_DUE_TO_GRAVITY = 9.8f;
        float length_units_per_meter = 128.f;
        b2SetLengthUnitsPerMeter(length_units_per_meter);
        b2WorldDef world_def = b2DefaultWorldDef();
        world_def.gravity.y = FORCE_DUE_TO_GRAVITY * length_units_per_meter;
        this->worldId = b2CreateWorld(&world_def);

        // Entity properties
        if (!this->entityTexture.loadfromImage(sdlHelper.renderer, "images/box.png")) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load box texture: %s\n", SDL_GetError());
        }

        b2Vec2 groundExtent = { 0.5f * 400, 0.5f * 248 };
        b2Vec2 boxExtent = { 0.5f * 400, 0.5f * 248 };
        //b2Polygon groundPolygon = b2MakeBox(groundExtent.x, groundExtent.y);
        b2Polygon boxPolygon = b2MakeBox(boxExtent.x, boxExtent.y);
        static constexpr auto BOX_ENTITIES = 15;
        this->entities.reserve(BOX_ENTITIES);
        int boxIndex = 0;
        for (auto i{ 0 }; i < 4; i++) {
            auto y = this->INIT_WINDOW_H - groundExtent.y - 100.f - (2.5f * static_cast<float>(i) + 2.f) * boxExtent.y - 20.f;
            for (auto j{ i }; j < 4; j++) {
                float x = 0.5f * this->INIT_WINDOW_W + (3.0f * j - i - 3.0f) * boxExtent.x;
                auto entity = std::make_unique<Entity>();
                b2BodyDef bodyDef = b2DefaultBodyDef();
                bodyDef.type = b2_dynamicBody;
                bodyDef.position = { x, y };
                entity->bodyId = b2CreateBody(this->worldId, &bodyDef);

                this->entities.push_back(std::move(entity));
                boxIndex++;
            }
        }
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

        static auto pushV = [&vertices](auto v1, auto v2, auto v3, auto v4)->void {
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

                if (mode == "backgrounds") {
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
}; // GameImpl


Game::Game(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<GameImpl>(std::cref(title), std::cref(version), w, h)} {
}

Game::~Game() {
    auto&& g = this->m_impl;
    // Clean up threads
    SDL_LockMutex(g->gameMtx);
    // Wake up any threads and wait for them to finish
    g->pendingWorkCount = 0;
    g->state = GameImpl::States::DONE;
    SDL_BroadcastCondition(g->gameCond);
    //SDL_SignalCondition(g->gameCond);
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

bool Game::run(const std::string& workerUrl, const std::string& lastSaveFile) const noexcept {

    using namespace std;

    // Create an alias to the game implementation
    auto&& gameImpl = this->m_impl;
    string_view titleView = gameImpl->title;
    gameImpl->sdlHelper.window = SDL_CreateWindow(titleView.data(), gameImpl->INIT_WINDOW_W, gameImpl->INIT_WINDOW_H, SDL_WINDOW_RESIZABLE);
    gameImpl->sdlHelper.renderer = SDL_CreateRenderer(gameImpl->sdlHelper.window, nullptr);

    SDL_Surface* icon = SDL_LoadBMP("images/icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(gameImpl->sdlHelper.window, icon);
        SDL_DestroySurface(icon);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s\n", SDL_GetError());
    }

    auto&& renderer = gameImpl->sdlHelper.renderer;
    SDL_SetRenderVSync(renderer, true);
    auto&& window = gameImpl->sdlHelper.window;

    // Load fonts and setup text rendering
    Game::GameImpl::SDLTexture fpsText;
    gameImpl->sdlHelper.loadFont("fonts/DMSans.ttf", 24);
    SDL_Color currentColor = { 255, 0, 175 };
    if (!fpsText.loadFromRenderedText(renderer, gameImpl->sdlHelper.font1, "FPS: 60", currentColor)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load text texture: %s\n", SDL_GetError());
        return false;
    }

    Game::GameImpl::SDLTexture renderToTexture;
    bool res = renderToTexture.loadTarget(renderer, gameImpl->INIT_WINDOW_W, gameImpl->INIT_WINDOW_H);
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load target texture: %s\n", SDL_GetError());
    }

    Game::GameImpl::SDLTexture tilemap;
    res = tilemap.loadfromImage(renderer, "images/tilemap.png");
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load tilemap texture: %s\n", SDL_GetError());
    }

    // Define the level by its vertices renderizable
    vector<SDL_Vertex> level;
    vector<shared_ptr<mazes::cell>> cells;
    // Create the physics world
    gameImpl->initEntities();

    // Timers for keeping track of frame rates
    double previous = SDL_GetTicks();
    double accumulator = 0.0, currentTimeStep = 0.0;
    // Game loop
    auto&& gState = gameImpl->state;
    while (gState != GameImpl::States::DONE) {
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = SDL_GetTicks() - previous;
        previous = SDL_GetTicks();

        accumulator += elapsed;
        while (accumulator >= FIXED_TIME_STEP) {
            // Input events
            gameImpl->sdlHelper.do_events(ref(gState));

            // Update
            accumulator -= elapsed;
            currentTimeStep += FIXED_TIME_STEP;

            b2World_Step(gameImpl->worldId, elapsed, 4);
        }

        // Update screen-related properties
        int display_w, display_h;
        SDL_GetWindowSize(window, &display_w, &display_h);
        float aspectRatioMin, aspectRatioMax;
        SDL_GetWindowAspectRatio(window, &aspectRatioMin, &aspectRatioMax);

        SDL_FPoint screenCenter = { static_cast<float>(display_w / 2), static_cast<float>(display_h / 2) };

        // Update FPS data - Check if it's been about a second
        static auto fpsCounter = 0;
        fpsCounter += 1;
        if (fpsCounter >= 60) {
            SDL_Log("FPS: %d\n", fpsCounter);
            SDL_Log("Frame Time / Update: %.3fms\n", static_cast<float>(elapsed) / static_cast<float>(fpsCounter));
            fpsCounter = 0;
        }

        SDL_SetRenderTarget(renderer, renderToTexture.get());

        // Render prep
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw/gen the level
        if (!gameImpl->pendingWorkCount) {
            if (gState == GameImpl::States::UPLOADING_LEVEL) {
                // Update state and current level
                SDL_Log("New level uploading\n");
                mazes::maze_builder builder;
                static constexpr auto INIT_MAZE_ROWS = 100, INIT_MAZE_COLS = 50;
                auto&& temp = builder.block_type(-1).rows(INIT_MAZE_ROWS).columns(INIT_MAZE_COLS).build();
                temp->compute_geometry();
                cells.clear();
                cells.reserve(INIT_MAZE_ROWS * INIT_MAZE_COLS);
                temp->populate_cells(ref(cells));
                assert(cells.size() > 0);
                level.clear();
                level.reserve(cells.size());

                SDL_FPoint cellSize = { static_cast<float>(display_w / temp->rows), static_cast<float>(display_h / temp->columns) };

                // Now start the worker threads
                gameImpl->genLevel(ref(level), cref(cells), cellSize);

                gState = GameImpl::States::PLAY;
            }

            // Now draw geometry ensuring complete render with no more work pending
            SDL_RenderGeometry(renderer, tilemap.get(), level.data(), level.size(), nullptr, 0);
        }

        // Entity rendering
        for (const auto& entity : gameImpl->entities) {
            b2Vec2 pos = b2Body_GetWorldPoint(entity->bodyId, { -entity->extent.x, -entity->extent.y });
            auto rotation = b2Body_GetRotation(entity->bodyId);
            auto angle = b2Rot_GetAngle(rotation);
            SDL_FRect renderQuad = { pos.x, pos.y, gameImpl->entityTexture.get()->w, gameImpl->entityTexture.get()->h };
            SDL_RenderTextureRotated(renderer, gameImpl->entityTexture.get(), &renderQuad, nullptr, angle, &screenCenter, SDL_FLIP_NONE);
        }

        // Finally, draw text to screen
        SDL_SetRenderDrawColor(renderer, 255, 0, 175, 255);
        fpsText.render(renderer, 25, 150);

        SDL_SetRenderTarget(renderer, nullptr);

        SDL_RenderTexture(renderer, renderToTexture.get(), nullptr, nullptr);

        SDL_RenderPresent(renderer);
    }

    return true;
} // run
