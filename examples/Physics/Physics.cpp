//
// @brief Physics class implementation
//  Simple 2D maze Physics using SDL3
//  Press 'B' to generate a new maze
// 
// Threading technique uses 'islands':
//  Example: https://github.com/SFML/SFML/tree/2.6.1/examples/island
//
// Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
//
//

#include "Physics.hpp"

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
#include <string_view>

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/maze_builder.h>

struct Physics::PhysicsImpl {

    enum class States {
        // Physics is starting, show welcome screen
        SPLASH,
        // Main menu / configurations 
        OPTIONS,
        // Physics is running
        PLAY,
        // Level is generated but Physics is paused/options
        PAUSE,
        // Physics is exiting and done
        DONE,
        // Useful when knowing when to re-draw in Physics loop
        // Level is being generated and not yet playable
        UPLOADING_LEVEL,
    };

    struct WorkItem {
        const std::string_view& mazeString;
        const SDL_FPoint cellSize;
        std::vector<SDL_Vertex>& vertices;
        int start, count;
        int rows, columns;  // Grid dimensions

        WorkItem(const std::string_view& mazeString, const SDL_FPoint cellSize,
            std::vector<SDL_Vertex>& vertices, int start, int count, int rows, int columns)
            : mazeString(mazeString), cellSize(cellSize)
            , vertices(vertices), start{ start }, count{ count }, rows{ rows }, columns{ columns } {
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
    // Keep track of user and Physics states
    States state;

    PhysicsImpl(const std::string& title, const std::string& version, int w, int h)
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

    ~PhysicsImpl() {

    }

    /**
     * @brief C-style function for SDL_Thread
     *  Handles when workers should process work items
     *
     */
    static int threadFunc(void* data) {
        using namespace std;
        auto* Physics = reinterpret_cast<PhysicsImpl*>(data);
        vector<SDL_Vertex> vertices;

        while (1) {
            {
                SDL_LockMutex(Physics->gameMtx);
                while (Physics->workQueue.empty() && Physics->state != States::DONE) {
                    SDL_WaitCondition(Physics->gameCond, Physics->gameMtx);
                }

                if (Physics->state == States::DONE) {
                    SDL_UnlockMutex(Physics->gameMtx);
                    break;
                }

                // Now process a work item
                if (!Physics->workQueue.empty()) {
                    
                    auto&& temp = Physics->workQueue.front();
                    Physics->workQueue.pop_front();
                    SDL_Log("Processing work item [ start: %d | count: %d | rows: %d | columns: %d]\n", temp.start, temp.count, temp.rows, temp.columns);
                    vertices.clear(); // Ensure the vector is empty before processing
                    Physics->doWork(ref(vertices), cref(temp));

                    SDL_Log("Generated %zu vertices for this work item\n", vertices.size());
                    
                    if (!vertices.empty()) {
                        copy(vertices.begin(), vertices.end(), back_inserter(temp.vertices));
                        SDL_Log("Total vertices after copy: %zu\n", temp.vertices.size());
                    } else {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No vertices generated for this work item\n");
                    }
                }
                SDL_UnlockMutex(Physics->gameMtx);
            }

            {
                // Update work count and wake up any threads waiting
                SDL_LockMutex(Physics->gameMtx);
                Physics->pendingWorkCount -= 1;
                SDL_Log("Pending work count: %d\n", Physics->pendingWorkCount);
                if (Physics->pendingWorkCount <= 0) {
                    SDL_SignalCondition(Physics->gameCond);
                }
                SDL_UnlockMutex(Physics->gameMtx);
            }
        }

        return 0;
    }

    /**
     * @brief Trigger work queue changes and signal threads
     *  Compute target block space for workers to process
     *  Construct all work items
     */
    void genLevel(std::vector<SDL_Vertex>& vertices, const std::string_view& mazeString, SDL_FPoint cellSize) noexcept {
        using namespace std;

        if (this->pendingWorkCount == 0) {
            SDL_WaitCondition(gameCond, gameMtx);
        }

        if (mazeString.empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty, cannot generate level\n");
            return;
        }

        // Log a sample of the maze string to verify its contents
        size_t sampleSize = std::min(size_t(100), mazeString.size());
        string sampleString = string(mazeString.substr(0, sampleSize));
        SDL_Log("Maze string begins with: '%s'\n", sampleString.c_str());
        SDL_Log("Total maze string length: %zu\n", mazeString.size());

        // Calculate maze dimensions
        size_t firstNewLine = mazeString.find('\n');
        if (firstNewLine == string::npos) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze format: no newlines found\n");
            return;
        }
        
        int columnsInMaze = static_cast<int>(firstNewLine);
        
        // Count rows
        int rowsInMaze = 1; // Start with 1 for the first line
        for (size_t i = 0; i < mazeString.size(); i++) {
            if (mazeString[i] == '\n') {
                rowsInMaze++;
            }
        }

        SDL_Log("Calculated maze dimensions: %d rows x %d columns\n", rowsInMaze, columnsInMaze);

        // Divide work among threads
        static constexpr auto BLOCK_COUNT = 4;
        
        // Each worker gets approximately equal sized chunks of the string
        size_t charsPerWorker = mazeString.size() / BLOCK_COUNT;
        
        // Clear the work queue first
        workQueue.clear();
        
        // Create work items
        for (auto w = 0; w < BLOCK_COUNT; w++) {
            size_t startIdx = w * charsPerWorker;
            size_t endIdx = (w == BLOCK_COUNT - 1) ? mazeString.size() : (w + 1) * charsPerWorker;
            
            // Ensure we don't split in the middle of a line
            if (w > 0) {
                while (startIdx > 0 && mazeString[startIdx] != '\n') {
                    startIdx--;
                }
                if (startIdx > 0) startIdx++; // Skip the newline
            }
            
            if (w < BLOCK_COUNT - 1) {
                while (endIdx < mazeString.size() && mazeString[endIdx] != '\n') {
                    endIdx++;
                }
                if (endIdx < mazeString.size()) endIdx++; // Include the newline
            }
            
            size_t count = endIdx - startIdx;
            SDL_Log("Worker %d: Processing from %zu to %zu (count: %zu)\n", w, startIdx, endIdx, count);
            
            workQueue.push_back({ 
                cref(mazeString), 
                cellSize, 
                ref(vertices), 
                static_cast<int>(startIdx), 
                static_cast<int>(count), 
                rowsInMaze, 
                columnsInMaze 
            });
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

        // Define colors
        SDL_FColor wallColor = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
        SDL_FColor cellColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // White

        const auto& mazeString = item.mazeString;
        const auto& cellSize = item.cellSize;
        SDL_Log("Processing maze string segment from %d to %d\n", item.start, item.start + item.count);
        
        // Log a sample of the maze string to see what we're working with
        std::string sampleString;
        if (!mazeString.empty()) {
            size_t sampleSize = std::min(size_t(100), mazeString.size());
            sampleString = std::string(mazeString.substr(0, sampleSize));
            SDL_Log("Maze string sample: '%s'\n", sampleString.c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty!\n");
            return;
        }
        
        // First pass: identify the maze dimensions
        // Find the number of columns by finding the first newline character
        size_t firstNewLine = mazeString.find('\n');
        if (firstNewLine == string::npos) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze format: no newlines found\n");
            return;
        }
        
        int columnsInMaze = static_cast<int>(firstNewLine);
        
        // Count total number of rows in the maze
        int rowsInMaze = 1; // Start with 1 for the first line
        for (size_t i = 0; i < mazeString.size(); i++) {
            if (mazeString[i] == '\n') {
                rowsInMaze++;
            }
        }

        SDL_Log("Maze dimensions: %d rows x %d columns\n", rowsInMaze, columnsInMaze);

        // Process only the part of the string assigned to this worker
        size_t startIdx = max(size_t(0), min(size_t(item.start), mazeString.size() - 1));
        size_t endIdx = min(mazeString.size(), startIdx + static_cast<size_t>(item.count));
        
        // Go back to beginning of line to ensure we don't start mid-cell
        while (startIdx > 0 && mazeString[startIdx] != '\n') {
            startIdx--;
        }
        if (startIdx > 0) startIdx++; // Skip the newline character
        
        // Go forward to end of line to ensure we don't end mid-cell
        while (endIdx < mazeString.size() && mazeString[endIdx] != '\n') {
            endIdx++;
        }
        
        string_view segment = mazeString.substr(startIdx, endIdx - startIdx);
        SDL_Log("Processing segment of length %zu\n", segment.size());
        
        // Parse maze format - each row has two parts: horizontal walls (+---+) and vertical walls (|   |)
        int currentRow = 0;
        int currentCol = 0;
        
        for (size_t i = 0; i < segment.size(); i++) {
            char c = segment[i];
            
            if (c == '\n') {
                // Move to the next row
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Calculate position for this character
            // Use fixed scaling to completely avoid infinity issues
            // Apply a view scaling factor to ensure vertices are visible in the viewport
            float scale = 10.0f; // Fixed scale that won't cause overflow
            float viewScaleX = static_cast<float>(INIT_WINDOW_W) / (static_cast<float>(columnsInMaze) * scale);
            float viewScaleY = static_cast<float>(INIT_WINDOW_H) / (static_cast<float>(rowsInMaze) * scale / 2.0f);
            float viewScale = std::min(viewScaleX, viewScaleY) * 0.9f; // 90% of available space for margins
            
            float x = static_cast<float>(currentCol) * scale * viewScale;
            float y = static_cast<float>(currentRow) * scale / 2.0f * viewScale;
            
            // Process different maze characters
            if (c == '+') {
                // Corner - just a reference point, don't render anything
                currentCol++;
            } else if (c == '-') {
                // Horizontal wall
                SDL_Vertex v1, v2, v3, v4;
                v1.position = { x, y };
                v2.position = { x + cellSize.x, y };
                v3.position = { x + cellSize.x, y + cellSize.y * 0.1f };
                v4.position = { x, y + cellSize.y * 0.1f };
                v1.color = v2.color = v3.color = v4.color = wallColor;
                v1.tex_coord = v2.tex_coord = v3.tex_coord = v4.tex_coord = { 0.0f, 0.0f };
                pushV(v1, v2, v3, v4);
                currentCol++;
            } else if (c == '|') {
                // Vertical wall
                SDL_Vertex v1, v2, v3, v4;
                v1.position = { x, y };
                v2.position = { x + cellSize.x * 0.1f, y };
                v3.position = { x + cellSize.x * 0.1f, y + cellSize.y };
                v4.position = { x, y + cellSize.y };
                v1.color = v2.color = v3.color = v4.color = wallColor;
                v1.tex_coord = v2.tex_coord = v3.tex_coord = v4.tex_coord = { 0.0f, 0.0f };
                pushV(v1, v2, v3, v4);
                currentCol++;
            } else if (c == ' ') {
                // Empty space, just increment the column
                currentCol++;
            }
        }
        
        // Second pass: draw cell backgrounds
        // Reset position tracking
        currentRow = 0;
        currentCol = 0;
        
        for (size_t i = 0; i < segment.size(); i++) {
            if (segment[i] == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // We're only interested in the cell spaces (between vertical walls)
            if (currentRow % 2 == 1 && segment[i] == ' ') {
                // This is inside a cell
                float x = static_cast<float>(currentCol) * cellSize.x;
                float y = static_cast<float>((currentRow - 1) / 2) * cellSize.y; // Convert back to cell coordinates
                
                // Draw cell background
                SDL_Vertex v1, v2, v3, v4;
                v1.position = { x, y };
                v2.position = { x + cellSize.x, y };
                v3.position = { x + cellSize.x, y + cellSize.y };
                v4.position = { x, y + cellSize.y };
                v1.color = v2.color = v3.color = v4.color = cellColor;
                v1.tex_coord = v2.tex_coord = v3.tex_coord = v4.tex_coord = { 0.0f, 0.0f };
                pushV(v1, v2, v3, v4);
            }
            
            currentCol++;
        }
    }
}; // PhysicsImpl


Physics::Physics(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<PhysicsImpl>(std::cref(title), std::cref(version), w, h)} {
}

Physics::~Physics() {
    auto&& g = this->m_impl;
    // Clean up threads
    SDL_LockMutex(g->gameMtx);
    // Wake up any threads and wait for them to finish
    g->pendingWorkCount = 0;
    g->state = PhysicsImpl::States::DONE;
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

bool Physics::run() const noexcept {

    using namespace std;

    // Create an alias to the Physics implementation
    auto&& sdlHelper = this->m_impl->sdlHelper;
    string_view titleView = this->m_impl->title;
    sdlHelper.window = SDL_CreateWindow(titleView.data(), this->m_impl->INIT_WINDOW_W, this->m_impl->INIT_WINDOW_H, SDL_WINDOW_RESIZABLE);
    if (!sdlHelper.window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    
    // Create the renderer with software rendering which is more compatible
    sdlHelper.renderer = SDL_CreateRenderer(sdlHelper.window, nullptr);
    if (!sdlHelper.renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdlHelper.window);
        return false;
    }
    
    // Check if renderer was created successfully
    if (auto props = SDL_GetRendererProperties(sdlHelper.renderer); props != 0) {
        SDL_Log("Renderer created: %s\n", SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "default"));
        // SDL_Log("Renderer flags: %u\n", info.flags);
        // SDL_Log("Max texture width: %d\n", info.max_texture_width);
        // SDL_Log("Max texture height: %d\n", info.max_texture_height);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get renderer info: %s\n", SDL_GetError());
    }

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

    Physics::PhysicsImpl::SDLTexture renderToTexture;
    bool res = renderToTexture.loadTarget(renderer, this->m_impl->INIT_WINDOW_W, this->m_impl->INIT_WINDOW_H);
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load target texture: %s\n", SDL_GetError());
    }

    // Define the level by its vertices renderizable
    vector<SDL_Vertex> level;
    string_view cells;

    // Timers for keeping track of frame rates
    double previous = SDL_GetTicks();
    double accumulator = 0.0, currentTimeStep = 0.0;
    // Physics loop
    auto&& gState = this->m_impl->state;
    while (gState != PhysicsImpl::States::DONE) {
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
        if (SDL_GetRenderTarget(renderer) != renderToTexture.get()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to set render target to texture\n");
            // Try again
            SDL_SetRenderTarget(renderer, renderToTexture.get());
        }

        // Render prep
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw/gen the level
        if (!this->m_impl->pendingWorkCount) {
            if (gState == PhysicsImpl::States::UPLOADING_LEVEL) {
                
                // Update state and current level

                static constexpr auto INIT_MAZE_ROWS = 100, INIT_MAZE_COLS = 50;
                auto maze_ptr = mazes::factory::create_q(INIT_MAZE_ROWS, INIT_MAZE_COLS);
                if (!maze_ptr.has_value()) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze with rows: %d and cols: %d\n", INIT_MAZE_ROWS, INIT_MAZE_COLS);
                } else {
                    SDL_Log("New level uploading with rows: %d and cols: %d\n", INIT_MAZE_ROWS, INIT_MAZE_COLS);

                    // Get the string representation of the maze
                    std::string mazeStr = mazes::stringz::stringify(maze_ptr.value());
                    
                    // Store the maze string in a more permanent storage since string_view only references it
                    static std::string persistentMazeStr;
                    persistentMazeStr = mazeStr; // Copy the string to ensure it stays in memory
                    cells = persistentMazeStr;   // Now cells references the persistent string
                    
                    SDL_Log("Generated maze string of size: %zu\n", cells.size());
                    if (cells.empty()) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Generated maze string is empty!\n");
                    } else {
                        // Log the first 200 characters to see what's in the string
                        std::string sample = persistentMazeStr.substr(0, std::min(size_t(200), persistentMazeStr.size()));
                        SDL_Log("Maze string starts with: '%s'\n", sample.c_str());
                    }
                    
                    assert(cells.size() > 0);

                    level.clear();
                    level.reserve(cells.size() * 24); // Each cell can have up to 4 walls, each wall is 6 vertices (2 triangles)
                    
                    // Get the actual rows and columns from the maze
                    auto rows = maze_ptr.value()->get_rows();
                    auto columns = maze_ptr.value()->get_columns();
                    
                    // Calculate appropriate cell size based on window dimensions and maze size
                    // Prevent division by zero which could cause infinities
                    auto safeColumns = std::max(1, columns);
                    auto safeRows = std::max(1, rows);
                    
                    SDL_FPoint cellSize = { 
                        static_cast<float>(display_w) / static_cast<float>(safeColumns), 
                        static_cast<float>(display_h) / static_cast<float>(safeRows) 
                    };
                    
                    // Validate cell size is reasonable to prevent inf/NaN
                    if (cellSize.x <= 0 || std::isinf(cellSize.x) || std::isnan(cellSize.x) || 
                        cellSize.y <= 0 || std::isinf(cellSize.y) || std::isnan(cellSize.y)) {
                        cellSize.x = 10.0f; // Default to reasonable values if calculation fails
                        cellSize.y = 10.0f;
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid cell size calculated: (%f, %f). Using defaults.\n", 
                            cellSize.x, cellSize.y);
                    }
                    
                    SDL_Log("Using cell size: (%f, %f) for grid of %d rows x %d columns\n", 
                        cellSize.x, cellSize.y, rows, columns);
    
                    // Now start the worker threads
                    this->m_impl->genLevel(ref(level), cref(cells), cellSize);
                }

                gState = PhysicsImpl::States::PLAY;
            }

            // Check SDL rendering state and texture validity
            SDL_Log("Render target texture valid: %s", renderToTexture.get() != nullptr ? "yes" : "no");
            SDL_Log("Current render target: %s", SDL_GetRenderTarget(renderer) == renderToTexture.get() ? "render texture" : "default");
            
            // Log viewport settings
            SDL_Rect v;
            SDL_GetRenderViewport(renderer, &v);
            SDL_Log("Viewport: x=%d, y=%d, w=%d, h=%d", v.x, v.y, v.w, v.h);
            
            // Now draw geometry ensuring complete render with no more work pending
            SDL_Log("Attempting to render %zu vertices\n", level.size());
            
            // Check if we have any vertices to render
            if (level.empty()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No vertices to render\n");
                // Draw a simple rectangle to show something on screen
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_FRect r = {10, 10, display_w - 20, display_h - 20};
                SDL_RenderFillRect(renderer, &r);
            } else {
                // Instead of using SDL_RenderGeometry which is having issues,
                // let's draw the maze using simple SDL drawing primitives
                
                // First draw a white background
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderClear(renderer);
                
                // Now parse the maze string directly and draw it
                int maxCols = 0;
                int maxRows = 0;
                
                // First, calculate maze dimensions
                const char* mazeData = cells.data();
                size_t mazeLen = cells.size();
                int currentRow = 0;
                int colCount = 0;
                
                for (size_t i = 0; i < mazeLen; i++) {
                    if (mazeData[i] == '\n') {
                        maxCols = std::max(maxCols, colCount);
                        colCount = 0;
                        currentRow++;
                    } else {
                        colCount++;
                    }
                }
                maxRows = currentRow + 1;
                
                // Calculate cell size to fit the window
                float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols + 1);
                float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows + 1);
                float cellSize = std::min(cellW, cellH);
                
                // Offset to center the maze
                float offsetX = (display_w - (maxCols * cellSize)) / 2.0f;
                float offsetY = (display_h - (maxRows * cellSize)) / 2.0f;
                
                SDL_Log("Drawing maze with dimensions: %d rows x %d cols", maxRows, maxCols);
                SDL_Log("Using cell size: %.2f with offsets: (%.2f, %.2f)", cellSize, offsetX, offsetY);
                
                // Now draw the maze
                currentRow = 0;
                int currentCol = 0;
                
                // Set to black for drawing walls
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                
                for (size_t i = 0; i < mazeLen; i++) {
                    char c = mazeData[i];
                    
                    if (c == '\n') {
                        currentCol = 0;
                        currentRow++;
                        continue;
                    }
                    
                    float x = offsetX + (currentCol * cellSize);
                    float y = offsetY + (currentRow * cellSize);
                    
                    // Draw based on character
                    if (c == '+') {
                        // Draw a small dot at the corner
                        SDL_FRect rect = {x - 1, y - 1, 3, 3};
                        SDL_RenderFillRect(renderer, &rect);
                    }
                    else if (c == '-') {
                        // Horizontal wall
                        SDL_RenderLine(renderer, 
                            x, y,
                            x + cellSize, y
                        );
                    }
                    else if (c == '|') {
                        // Vertical wall
                        SDL_RenderLine(renderer,
                            x, y,
                            x, y + cellSize
                        );
                    }
                    
                    currentCol++;
                }
                
                SDL_Log("Maze drawing completed successfully");
            }
        }

        // Finally, draw text to screen
        SDL_SetRenderDrawColor(renderer, 255, 0, 175, 255);

        SDL_SetRenderTarget(renderer, nullptr);

        SDL_RenderTexture(renderer, renderToTexture.get(), nullptr, nullptr);

        SDL_RenderPresent(renderer);
    }

    return true;
} // run
