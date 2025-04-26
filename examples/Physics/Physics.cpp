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

#include <cmath>
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

#include <box2d/box2d.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/maze_builder.h>

static constexpr auto INIT_MAZE_ROWS = 10, INIT_MAZE_COLS = 10;

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
    
    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f; // Number of hits before a wall breaks
    static constexpr float BALL_RADIUS = 0.45f; // Reduced from 0.2f to make balls smaller
    static constexpr float WALL_WIDTH = 0.1f;
    static constexpr int MAX_BALLS = 10;
    
    // Maze rendering variables
    float cellSize = 0.0f;    // Size of a maze cell in pixels
    float offsetX = 0.0f;     // X offset for centering the maze
    float offsetY = 0.0f;     // Y offset for centering the maze
    int maxCols = 0;          // Number of columns in the maze
    int maxRows = 0;          // Number of rows in the maze
    
    struct Wall {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        int hitCount = 0;
        bool isDestroyed = false;
        int row, col;         // Added to track wall position in the maze
        char type;            // '-', '|', or '+' to determine wall type
    };
    
    struct Ball {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        bool isActive = false;
        bool isDragging = false;
        bool isExploding = false;
        float explosionTimer = 0.0f;
    };
    
    struct ExitCell {
        int row;
        int col;
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        int ballsCollected = 0;
    };
        
    // Box2D world and physics components
    b2WorldId physicsWorldId = b2_nullWorldId;
    float timeStep = 1.0f / 60.0f;
    int32_t velocityIterations = 6;
    int32_t positionIterations = 2;
    
    // Game-specific variables
    std::vector<Wall> walls;
    std::vector<Ball> balls;
    ExitCell exitCell;
    int score = 0;
    float pixelsPerMeter = 10.0f; // Scale factor for Box2D (which uses meters)
    bool isDragging = false;
    int draggedBallIndex = -1;
    b2Vec2 lastMousePos = {0.0f, 0.0f};

    struct WorkItem {
        const std::string_view& mazeString;
        const SDL_FPoint cellSize;
        std::vector<SDL_Vertex>& vertices;
        int start, count;
        int rows, columns;
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

        void do_events(States& state, OrthographicCamera& camera) noexcept {
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
                    // Arrow key controls for camera movement
                    else if (e.key.scancode == SDL_SCANCODE_LEFT) {
                        camera.x += camera.panSpeed * 2.0f;
                        SDL_Log("Camera moved left: x=%.2f, y=%.2f", camera.x, camera.y);
                    }
                    else if (e.key.scancode == SDL_SCANCODE_RIGHT) {
                        camera.x -= camera.panSpeed * 2.0f;
                        SDL_Log("Camera moved right: x=%.2f, y=%.2f", camera.x, camera.y);
                    }
                    else if (e.key.scancode == SDL_SCANCODE_UP) {
                        camera.y += camera.panSpeed * 2.0f;
                        SDL_Log("Camera moved up: x=%.2f, y=%.2f", camera.x, camera.y);
                    }
                    else if (e.key.scancode == SDL_SCANCODE_DOWN) {
                        camera.y -= camera.panSpeed * 2.0f;
                        SDL_Log("Camera moved down: x=%.2f, y=%.2f", camera.x, camera.y);
                    }
                    // Rotation controls with Q/E keys
                    else if (e.key.scancode == SDL_SCANCODE_Q) {
                        camera.rotation -= camera.rotationSpeed * 2.0f;
                        SDL_Log("Camera rotated counter-clockwise: rotation=%.2f", camera.rotation);
                    }
                    else if (e.key.scancode == SDL_SCANCODE_E) {
                        camera.rotation += camera.rotationSpeed * 2.0f;
                        SDL_Log("Camera rotated clockwise: rotation=%.2f", camera.rotation);
                    }
                    // Zoom controls with +/- (equals/minus) keys
                    else if (e.key.scancode == SDL_SCANCODE_EQUALS) { // + key (may require shift)
                        camera.zoom *= (1.0f + camera.zoomSpeed);
                        SDL_Log("Camera zoomed in: zoom=%.2f", camera.zoom);
                    }
                    else if (e.key.scancode == SDL_SCANCODE_MINUS) { // - key
                        camera.zoom /= (1.0f + camera.zoomSpeed);
                        SDL_Log("Camera zoomed out: zoom=%.2f", camera.zoom);
                    }
                    // Reset camera with R key
                    else if (e.key.scancode == SDL_SCANCODE_R) {
                        camera.x = 0.0f;
                        camera.y = 0.0f;
                        camera.zoom = 1.0f;
                        camera.rotation = 0.0f;
                        SDL_Log("Camera reset to default position and orientation");
                    }
                } else if (e.type == SDL_EVENT_QUIT) {
                    state = States::DONE;
                    break;
                }
            }
            
            // Also check for continuous key presses for smoother camera movement
            const auto* keyState = SDL_GetKeyboardState(NULL);
            if (keyState[SDL_SCANCODE_LEFT]) {
                camera.x += camera.panSpeed;
            }
            if (keyState[SDL_SCANCODE_RIGHT]) {
                camera.x -= camera.panSpeed;
            }
            if (keyState[SDL_SCANCODE_UP]) {
                camera.y += camera.panSpeed;
            }
            if (keyState[SDL_SCANCODE_DOWN]) {
                camera.y -= camera.panSpeed;
            }
            
            // Add continuous rotation and zoom (for smoother control)
            if (keyState[SDL_SCANCODE_Q]) {
                camera.rotation -= camera.rotationSpeed;
            }
            if (keyState[SDL_SCANCODE_E]) {
                camera.rotation += camera.rotationSpeed;
            }
            if (keyState[SDL_SCANCODE_EQUALS]) { // + key
                camera.zoom *= (1.0f + camera.zoomSpeed * 0.005f); // gentler continuous zoom
            }
            if (keyState[SDL_SCANCODE_MINUS]) { // - key
                camera.zoom /= (1.0f + camera.zoomSpeed * 0.005f); // gentler continuous zoom
            }
            
            // Enforce zoom limits to prevent extreme values
            camera.zoom = max(0.1f, min(5.0f, camera.zoom));
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
    std::deque<WorkItem> workQueue;
    std::vector<SDL_Thread*> threads;
    SDL_Mutex* gameMtx;
    SDL_Condition* gameCond;
    SDLTexture entityTexture;
    int pendingWorkCount;
    States state;

    // Camera for coordinate transformations
    OrthographicCamera camera;

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

    void genLevel(std::vector<SDL_Vertex>& vertices, const std::string_view& mazeString, SDL_FPoint cellSize) noexcept {
        using namespace std;
        if (this->pendingWorkCount == 0) {
            SDL_WaitCondition(gameCond, gameMtx);
        }
        if (mazeString.empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty, cannot generate level\n");
            return;
        }
        SDL_Log("Maze string begins with: '%s'\n", mazeString.substr(0, 20));
        SDL_Log("Total maze string length: %zu\n", mazeString.size());
        size_t firstNewLine = mazeString.find('\n');
        if (firstNewLine == string::npos) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze format: no newlines found\n");
            return;
        }
        int columnsInMaze = static_cast<int>(firstNewLine);
        int rowsInMaze = 1;
        for (size_t i = 0; i < mazeString.size(); i++) {
            if (mazeString[i] == '\n') {
                rowsInMaze++;
            }
        }
        SDL_Log("Calculated maze dimensions: %d rows x %d columns\n", rowsInMaze, columnsInMaze);
        static constexpr auto BLOCK_COUNT = 4;
        size_t charsPerWorker = mazeString.size() / BLOCK_COUNT;
        workQueue.clear();
        for (auto w = 0; w < BLOCK_COUNT; w++) {
            size_t startIdx = w * charsPerWorker;
            size_t endIdx = (w == BLOCK_COUNT - 1) ? mazeString.size() : (w + 1) * charsPerWorker;
            if (w > 0) {
                while (startIdx > 0 && mazeString[startIdx] != '\n') {
                    startIdx--;
                }
                if (startIdx > 0) startIdx++;
            }
            if (w < BLOCK_COUNT - 1) {
                while (endIdx < mazeString.size() && mazeString[endIdx] != '\n') {
                    endIdx++;
                }
                if (endIdx < mazeString.size()) endIdx++;
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

    void doWork(std::vector<SDL_Vertex>& vertices, const WorkItem& item) {
        using namespace std;
        SDL_FColor wallColor = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
        auto pushV = [&vertices](auto v1, auto v2, auto v3, auto v4)->void {
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v4);
            vertices.push_back(v2);
            vertices.push_back(v3);
            vertices.push_back(v4);
        };
        SDL_FColor cellColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // White
        const auto& mazeString = item.mazeString;
        const auto& cellSize = item.cellSize;
        SDL_Log("Processing maze string segment from %d to %d\n", item.start, item.start + item.count);
        std::string sampleString;
        if (!mazeString.empty()) {
            size_t sampleSize = std::min(size_t(100), mazeString.size());
            sampleString = std::string(mazeString.substr(0, sampleSize));
            SDL_Log("Maze string sample: '%s'\n", sampleString.c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty!\n");
            return;
        }
        size_t firstNewLine = mazeString.find('\n');
        if (firstNewLine == string::npos) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze format: no newlines found\n");
            return;
        }
        size_t startIdx = max(size_t(0), min(size_t(item.start), mazeString.size() - 1));
        int columnsInMaze = static_cast<int>(firstNewLine);
        int rowsInMaze = 1;
        for (size_t i = 0; i < mazeString.size(); i++) {
            if (mazeString[i] == '\n') {
                rowsInMaze++;
            }
        }
        SDL_Log("Maze dimensions: %d rows x %d columns\n", rowsInMaze, columnsInMaze);
        size_t endIdx = min(mazeString.size(), startIdx + static_cast<size_t>(item.count));
        while (startIdx > 0 && mazeString[startIdx] != '\n') {
            startIdx--;
        }
        if (startIdx > 0) startIdx++;
        while (endIdx < mazeString.size() && mazeString[endIdx] != '\n') {
            endIdx++;
        }
        string_view segment = mazeString.substr(startIdx, endIdx - startIdx);
        SDL_Log("Processing segment of length %zu\n", segment.size());
        int currentRow = 0;
        int currentCol = 0;
        
        // Calculate the proper offsets for centering the maze
        float mazeWidth = columnsInMaze * cellSize.x;
        float mazeHeight = rowsInMaze * cellSize.y;
        float offsetX = (static_cast<float>(INIT_WINDOW_W) - mazeWidth) / 2.0f;
        float offsetY = (static_cast<float>(INIT_WINDOW_H) - mazeHeight) / 2.0f;
        
        // Ensure offsets are never negative
        offsetX = std::max(0.0f, offsetX);
        offsetY = std::max(0.0f, offsetY);

        for (size_t i = 0; i < segment.size(); i++) {
            char c = segment[i];
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Calculate position with proper offsets for centering
            float x = offsetX + static_cast<float>(currentCol) * cellSize.x;
            float y = offsetY + static_cast<float>(currentRow) * cellSize.y;
            
            if (c == '+') {
                currentCol++;
            } else if (c == '-') {
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
                currentCol++;
            }
        }
        
        currentRow = 0;
        currentCol = 0;
        for (size_t i = 0; i < segment.size(); i++) {
            if (segment[i] == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            if (currentRow % 2 == 1 && segment[i] == ' ') {
                // Calculate position with proper offsets for centering
                float x = offsetX + static_cast<float>(currentCol) * cellSize.x;
                float y = offsetY + static_cast<float>(currentRow) * cellSize.y;
                
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

    // Initialize the Box2D physics world
    void initPhysics() {
        // Set length units per meter as recommended in Box2D FAQ
        // This gives us a better scale for the simulation
        // Box2D works best with moving objects between 0.1 and 10 meters in size
        float lengthUnitsPerMeter = 1.0f;
        b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);
    
        b2WorldDef worldDef = b2DefaultWorldDef();
    
        // Use a realistic but slightly reduced gravity for better gameplay
        worldDef.gravity.y = 9.8f;
        
        // Create physics world
        physicsWorldId = b2CreateWorld(&worldDef);
        
        // Clear any existing entities
        walls.clear();
        balls.clear();
        
        // Set good values for physics simulation
        timeStep = 1.0f / 60.0f;        // Simulate at 60Hz
        pixelsPerMeter = 40.0f;         // Good scaling factor for visibility
        
        SDL_Log("Box2D physics world initialized with gravity: %f", worldDef.gravity.y);
    }
    
    // Convert screen coordinates to physics world coordinates
    b2Vec2 screenToPhysics(float screenX, float screenY) {
        // First account for camera transformation
        float worldX, worldY;
        int display_w, display_h;
        SDL_GetWindowSize(sdlHelper.window, &display_w, &display_h);
        camera.screenToWorld(screenX, screenY, worldX, worldY, display_w, display_h);
        
        // Then convert from world to physics coordinates by accounting for offset and scale
        float physX = (worldX - offsetX) / pixelsPerMeter;
        float physY = (worldY - offsetY) / pixelsPerMeter;
        
        return {physX, physY};
    }
    
    // Convert physics world coordinates to screen coordinates
    SDL_FPoint physicsToScreen(float physX, float physY) {
        return {physX * pixelsPerMeter, physY * pixelsPerMeter};
    }
    
    // Create a ball at the specified position
    Ball createBall(float x, float y) {
        // Create a dynamic body for the ball
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {x, y};
        bodyDef.linearVelocity = {
            (float)((rand() % 100) - 50) / 30.0f,  // Increased initial velocity
            (float)((rand() % 100) - 50) / 30.0f   // for better collisions
        };
        bodyDef.linearDamping = 0.2f;  // Reduced damping for more movement
        bodyDef.angularDamping = 0.4f; // Reduced spinning damping
        bodyDef.isBullet = true;       // Enable continuous collision detection
        bodyDef.userData = reinterpret_cast<void*>(this);
        
        b2BodyId ballBodyId = b2CreateBody(physicsWorldId, &bodyDef);
        
        // Explicitly set the body to be awake
        b2Body_SetAwake(ballBodyId, true);
        
        // Create a circle shape for the ball
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.5f;  // Heavier balls for better collision impacts
        shapeDef.material.rollingResistance = 0.1f; // Lower resistance for smoother rolling
        shapeDef.material.friction = 0.2f; // Lower friction
        shapeDef.material.restitution = 0.8f; // Higher restitution for more bounce
        
        // In Box2D 3.1.0, the circle is defined separately from the shape def
        b2Circle circle = {{0.f, 0.f}, BALL_RADIUS};
        
        b2ShapeId ballShapeId = b2CreateCircleShape(ballBodyId, &shapeDef, &circle);
        
        // Return a new ball with this body
        Ball newBall;
        newBall.bodyId = ballBodyId;
        newBall.shapeId = ballShapeId;
        newBall.isActive = true;
        
        return newBall;
    }

    // Convert the ASCII maze into Box2D physics objects
    void createMazePhysics(const std::string_view& mazeString, float cellSize) {
        // Clear any existing physics objects
        if (B2_IS_NON_NULL(physicsWorldId)) {
            b2DestroyWorld(physicsWorldId);
        }
        
        // Create a new physics world
        initPhysics();
        walls.clear();
        balls.clear();
        
        // Calculate maze dimensions
        int maxCols = 0;
        int maxRows = 0;
        
        // First, calculate maze dimensions
        const char* mazeData = mazeString.data();
        size_t mazeLen = mazeString.size();
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
        
        SDL_Log("Maze dimensions: %d rows x %d columns", maxRows, maxCols);
        
        // Store the maze dimensions
        this->maxCols = maxCols;
        this->maxRows = maxRows;
        
        // Calculate cell dimensions based on the number of cells
        float cellWidth = cellSize;
        float cellHeight = cellSize;
        
        // Store cell size and calculate offsets for centering
        this->cellSize = cellSize;
        this->pixelsPerMeter = cellSize;
        
        // Center the maze in the display (calculate offsets)
        float mazeWidth = maxCols * cellSize;
        float mazeHeight = maxRows * cellSize;
        this->offsetX = (INIT_WINDOW_W - mazeWidth) / 2.0f;
        this->offsetY = (INIT_WINDOW_H - mazeHeight) / 2.0f;
        
        // Ensure offsets are never negative
        this->offsetX = std::max(0.0f, this->offsetX);
        this->offsetY = std::max(0.0f, this->offsetY);
        
        // Reset tracking variables
        currentRow = 0;
        int currentCol = 0;
        
        // Create world boundaries to prevent balls from escaping the screen
        float worldWidth = (maxCols * cellWidth) / pixelsPerMeter;
        float worldHeight = (maxRows * cellHeight) / pixelsPerMeter;
        
        SDL_Log("Physics world size: %.2f x %.2f meters", worldWidth, worldHeight);
        SDL_Log("Cell size: %.2f pixels, pixelsPerMeter: %.2f", cellSize, pixelsPerMeter);
        
        // Add boundary walls (top, bottom, left, right)
        b2BodyDef boundaryDef = b2DefaultBodyDef();
        boundaryDef.type = b2_staticBody;
        boundaryDef.userData = reinterpret_cast<void*>(3000); // Special identifier for boundaries
        b2BodyId boundaryBodyId = b2CreateBody(physicsWorldId, &boundaryDef);
        
        b2ShapeDef boundaryShapeDef = b2DefaultShapeDef();
        boundaryShapeDef.density = 0.0f;
        boundaryShapeDef.material.friction = 0.3f;
        boundaryShapeDef.material.restitution = 0.8f;
        
        float worldLeft = -1.0f;
        float worldRight = worldWidth + 1.0f;
        float worldTop = -1.0f;
        float worldBottom = worldHeight + 1.0f;
        
        // Top boundary
        {
            b2Segment segment = {{worldLeft, worldTop}, {worldRight, worldTop}};
            b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Bottom boundary
        {
            b2Segment segment = {{worldLeft, worldBottom}, {worldRight, worldBottom}};
            b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Left boundary
        {
            b2Segment segment = {{worldLeft, worldTop}, {worldLeft, worldBottom}};
            b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Right boundary
        {
            b2Segment segment = {{worldRight, worldTop}, {worldRight, worldBottom}};
            b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Now create walls for maze structure using a more precise approach
        for (size_t i = 0; i < mazeLen; i++) {
            char c = mazeData[i];
            
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Convert to physics coordinates - position is center of the cell
            float physX = (currentCol * cellWidth) / pixelsPerMeter;
            float physY = (currentRow * cellHeight) / pixelsPerMeter;
            
            // Create walls for different maze characters
            if (c == '-' || c == '|' || c == '+') {
                b2BodyDef wallDef = b2DefaultBodyDef();
                wallDef.type = b2_staticBody;
                wallDef.position = {physX, physY};
                
                // Store the wall index in the user data to identify it later
                int wallIndex = walls.size();
                // We need to store our wall index in the userData
                wallDef.userData = reinterpret_cast<void*>(1000 + wallIndex); // Use offset to identify as wall
                
                b2BodyId wallBodyId = b2CreateBody(physicsWorldId, &wallDef);
                
                // Explicitly set the body to be awake using the proper API function
                b2Body_SetAwake(wallBodyId, true);
                
                // Create shape definition with improved properties
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = 0.0f;  // static body
                shapeDef.material.friction = 0.4f; // Higher friction
                shapeDef.material.restitution = 0.4f; // Moderate bounce
                shapeDef.material.rollingResistance = 1.0f; // Solid walls
                
                // Adjust wall size based on character
                if (c == '-') {
                    // Horizontal wall - width of full cell, height is smaller
                    float halfWidth = (cellWidth / pixelsPerMeter) * 0.5f;   // Full width of cell
                    float halfHeight = (cellHeight / pixelsPerMeter) * 0.1f; // 20% of cell height (centered)
                    
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2ShapeId wallShapeId = b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    Wall wall;
                    wall.bodyId = wallBodyId;
                    wall.shapeId = wallShapeId;
                    wall.hitCount = 0;
                    wall.isDestroyed = false;
                    wall.row = currentRow;
                    wall.col = currentCol;
                    wall.type = '-';
                    walls.push_back(wall);
                    
                    SDL_Log("Created horizontal wall at (%d,%d) position (%.2f,%.2f)", 
                        currentRow, currentCol, physX, physY);
                } 
                else if (c == '|') {
                    // Vertical wall - height of full cell, width is smaller
                    float halfWidth = (cellWidth / pixelsPerMeter) * 0.1f;   // 20% of cell width (centered)
                    float halfHeight = (cellHeight / pixelsPerMeter) * 0.5f; // Full height of cell
                    
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2ShapeId wallShapeId = b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    Wall wall;
                    wall.bodyId = wallBodyId;
                    wall.shapeId = wallShapeId;
                    wall.hitCount = 0;
                    wall.isDestroyed = false;
                    wall.row = currentRow;
                    wall.col = currentCol;
                    wall.type = '|';
                    walls.push_back(wall);
                    
                    SDL_Log("Created vertical wall at (%d,%d) position (%.2f,%.2f)",
                        currentRow, currentCol, physX, physY);
                }
                else if (c == '+') {
                    // Junction/corner - create a small square
                    float halfSize = (cellWidth / pixelsPerMeter) * 0.15f;  // 30% of cell size (squared)
                    
                    b2Polygon boxShape = b2MakeBox(halfSize, halfSize);
                    b2ShapeId wallShapeId = b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    Wall wall;
                    wall.bodyId = wallBodyId;
                    wall.shapeId = wallShapeId;
                    wall.hitCount = 0;
                    wall.isDestroyed = false;
                    wall.row = currentRow;
                    wall.col = currentCol;
                    wall.type = '+';
                    walls.push_back(wall);
                    
                    SDL_Log("Created corner wall at (%d,%d) position (%.2f,%.2f)",
                        currentRow, currentCol, physX, physY);
                }
            }
            
            currentCol++;
        }
        
        // Find all spaces where numbers (0-9) should be in the reference maze
        // These are typically at odd rows and columns (1-based indexing)
        std::vector<std::pair<int, int>> ballPositions;
        
        currentRow = 0;
        currentCol = 0;
        for (size_t i = 0; i < mazeLen; i++) {
            char c = mazeData[i];
            
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Look for empty spaces in paths - these are where balls should go
            // Corresponding to positions 0-9 in the reference maze
            if (c == ' ' && currentRow % 2 == 1 && currentCol % 2 == 1) {
                ballPositions.push_back({currentRow, currentCol});
            }
            
            currentCol++;
        }
        
        // Create the exit - place it at the top-left corner (to match cell 'P' in reference maze)
        // Find first open space in the maze
        int exitRow = -1, exitCol = -1;
        currentRow = 1;  // Start at row 1 (first path row)
        currentCol = 1;  // Start at col 1 (first path column)
        
        // If we found the top-left corner, use it for the exit
        if (!ballPositions.empty()) {
            exitRow = ballPositions[0].first;
            exitCol = ballPositions[0].second;
        } else {
            // Fallback to a reasonable position if we can't find spaces
            exitRow = 1;
            exitCol = 1;
        }
        
        // Create the exit at the found position
        exitCell.row = exitRow;
        exitCell.col = exitCol;
        
        b2BodyDef exitDef = b2DefaultBodyDef();
        exitDef.type = b2_staticBody;
        exitDef.position = {
            (exitCol * cellWidth) / pixelsPerMeter,
            (exitRow * cellHeight) / pixelsPerMeter
        };
        // Store special ID for exit
        exitDef.userData = reinterpret_cast<void*>(2000); // Use offset to identify as exit
        
        exitCell.bodyId = b2CreateBody(physicsWorldId, &exitDef);
        
        // Create circle shape for exit
        b2ShapeDef circleDef = b2DefaultShapeDef();
        circleDef.density = 0.0f;  // static body
        circleDef.isSensor = true; // Make it a sensor so balls can pass through
        
        // Make exit circle size appropriate for the cell
        float exitRadius = cellWidth / (3.0f * pixelsPerMeter);
        b2Circle exitCircle = {{0.f, 0.f}, exitRadius};
        
        exitCell.shapeId = b2CreateCircleShape(exitCell.bodyId, &circleDef, &exitCircle);
        
        // Create balls at the remaining positions (numbers 0-9 in reference maze)
        // Start from the end (highest number) and work backwards
        // Skip the first position as that's where the exit is
        int numBalls = std::min(10, (int)ballPositions.size() - 1);
        
        for (int i = 0; i < numBalls; i++) {
            // Get position from the end of the list (to match highest numbers first)
            int ballIndex = ballPositions.size() - 1 - i;
            if (ballIndex <= 0) break; // Skip the exit position
            
            auto [row, col] = ballPositions[ballIndex];
            
            float ballX = (col * cellWidth) / pixelsPerMeter;
            float ballY = (row * cellHeight) / pixelsPerMeter;
            
            balls.push_back(createBall(ballX, ballY));
            
            SDL_Log("Created ball at (%d,%d) position (%.2f,%.2f)",
                row, col, ballX, ballY);
        }
        
        SDL_Log("Maze physics created with %zu walls and %zu balls", walls.size(), balls.size());
        SDL_Log("Exit placed at row %d, col %d", exitRow, exitCol);
    }

    // Utility method for handling wall collisions
    void handleWallCollision(b2BodyId possibleWallId, b2BodyId possibleBallId) {
        // If this is a ball hitting a wall
        void* wallUserData = b2Body_GetUserData(possibleWallId);
        void* ballUserData = b2Body_GetUserData(possibleBallId);
        
        // Check if it's a wall by checking the user data range
        // Walls are stored with pointer values starting at 1000
        uintptr_t wallValue = reinterpret_cast<uintptr_t>(wallUserData);
        
        // Make sure this is a ball hitting a wall (not another wall or object)
        if (wallValue >= 1000 && wallValue < 2000) {
            // This is a wall - find its index
            int wallIndex = static_cast<int>(wallValue - 1000);
            
            // Make sure the index is valid
            if (wallIndex >= 0 && wallIndex < walls.size()) {
                Wall& wall = walls[wallIndex];
                
                if (wall.isDestroyed) {
                    return; // Skip if wall is already flagged for destruction
                }
                
                // Get ball velocity to ensure only significant impacts count
                b2Vec2 ballVel = b2Body_GetLinearVelocity(possibleBallId);
                float impactSpeed = b2Length(ballVel);
                
                // Only count the hit if it's a significant impact
                if (impactSpeed > 1.5f) { // Threshold for counting a hit
                    wall.hitCount++;
                    SDL_Log("Wall hit! Wall index: %d, Hit count: %d/%d, Impact speed: %.2f", 
                           wallIndex, wall.hitCount, (int)WALL_HIT_THRESHOLD, impactSpeed);
                    
                    // Apply a small impulse to make the hit feel more impactful
                    b2Vec2 normalizedVel = ballVel;
                    if (impactSpeed > 0) {
                        normalizedVel = normalizedVel * (1.0f / impactSpeed);
                    }
                    // Add some randomness to the bounce for more dynamic behavior
                    float randomAngle = ((rand() % 20) - 10) * 0.01f; // Small random angle adjustment
                    float cosR = cosf(randomAngle);
                    float sinR = sinf(randomAngle);
                    b2Vec2 adjustedDir = {
                        normalizedVel.x * cosR - normalizedVel.y * sinR,
                        normalizedVel.x * sinR + normalizedVel.y * cosR
                    };
                    
                    // Create a more dynamic collision response
                    b2Vec2 opposingForce = adjustedDir * -0.7f * impactSpeed; // Scale with impact speed
                    b2Body_ApplyLinearImpulseToCenter(possibleBallId, opposingForce, true);
                    
                    // Increase the ball's angular velocity for more interesting physics
                    float spin = (rand() % 10) * 0.3f;
                    b2Body_ApplyAngularImpulse(possibleBallId, spin, true);
                }
                
                // Check if wall should break
                if (wall.hitCount >= WALL_HIT_THRESHOLD && !wall.isDestroyed) {
                    wall.isDestroyed = true;
                    SDL_Log("Wall %d destroyed after %d hits!", wallIndex, wall.hitCount);
                    
                    // Increment score
                    score += 10;
                    
                    // IMPORTANT: Do NOT destroy the body here - let updatePhysicsObjects do it
                    // This fixes the timing issue between physics and rendering
                    // The wall will be visually marked as destroyed but the body remains valid
                    // until the next physics update cycle where it will be properly removed
                }
            }
        }
    }
    
    // Utility method for handling ball-to-ball collisions
    void handleBallCollision(b2BodyId bodyAId, b2BodyId bodyBId) {
        // Check if both are balls (balls have user data pointer == this)
        auto* userDataA = b2Body_GetUserData(bodyAId);
        auto* userDataB = b2Body_GetUserData(bodyBId);
        
        if (userDataA == reinterpret_cast<void*>(this) && 
            userDataB == reinterpret_cast<void*>(this)) {
            
            // Find the corresponding ball objects
            Ball* ballA = nullptr;
            Ball* ballB = nullptr;
            
            for (Ball& ball : balls) {
                if (b2Body_GetPosition(bodyAId).x == b2Body_GetPosition(ball.bodyId).x
                && b2Body_GetPosition(bodyAId).y == b2Body_GetPosition(ball.bodyId).y ) {
                    ballA = &ball;
                }

                if (b2Body_GetPosition(bodyBId).x == b2Body_GetPosition(ball.bodyId).x
                && b2Body_GetPosition(bodyBId).y == b2Body_GetPosition(ball.bodyId).y ) {
                    ballB = &ball;
                }
            }
            
            if (ballA && ballB) {
                // Start explosion animation for both balls
                ballA->isExploding = true;
                ballB->isExploding = true;
            }
        }
    }

    // Handle ball dragging with improved interaction
    void updateBallDrag(float mouseX, float mouseY, bool isMouseDown) {
        // Convert screen coordinates to physics world coordinates
        b2Vec2 mousePhysicsPos = screenToPhysics(mouseX, mouseY);
        
        // If mouse button is pressed
        if (isMouseDown) {
            if (!isDragging) {
                // Try to find a ball to drag - search from front to back for better selection
                for (int i = 0; i < balls.size(); i++) {
                    auto& ball = balls[i];
                    
                    if (!ball.isActive || ball.isExploding)
                        continue;
                    
                    // Get ball position and compare to mouse
                    b2Vec2 ballPos = b2Body_GetPosition(ball.bodyId);
                    float distance = b2Distance(mousePhysicsPos, ballPos);
                    
                    // If mouse is over this ball with improved hit detection radius
                    if (distance <= BALL_RADIUS * 2.0f) {
                        // Start dragging this ball
                        isDragging = true;
                        draggedBallIndex = i;
                        lastMousePos = mousePhysicsPos;
                        ball.isDragging = true;
                        
                        // Wake up the body explicitly to ensure it responds to forces
                        b2Body_SetAwake(ball.bodyId, true);
                        
                        // Apply a small impulse to "pick up" the ball
                        b2Vec2 impulse = {0.0f, -0.5f};
                        b2Body_ApplyLinearImpulseToCenter(ball.bodyId, impulse, true);
                        
                        SDL_Log("Ball %d selected for dragging at physics pos (%.2f, %.2f)", 
                                i, ballPos.x, ballPos.y);
                        break;
                    }
                }
            }
            else if (draggedBallIndex >= 0 && draggedBallIndex < balls.size()) {
                // Continue dragging the selected ball
                auto& ball = balls[draggedBallIndex];
                
                if (ball.isActive && !ball.isExploding) {
                    // Get ball position
                    b2Vec2 ballPos = b2Body_GetPosition(ball.bodyId);
                    
                    // Calculate direct vector to target position
                    b2Vec2 toTarget = mousePhysicsPos - ballPos;
                    
                    // Use a mouse joint effect:
                    // 1. Apply stronger force for more responsive dragging
                    float forceScale = 220.0f;
                    b2Vec2 force = toTarget * forceScale;
                    
                    // Apply force to the ball center
                    b2Body_ApplyForceToCenter(ball.bodyId, force, true);
                    
                    // 2. Set a target velocity for more direct control
                    float speedFactor = 15.0f; // Increased for more responsiveness
                    b2Vec2 targetVelocity = toTarget * speedFactor;
                    
                    // Limit max velocity for stability
                    float maxSpeed = 25.0f; // Increased for better response
                    float currentSpeed = b2Length(targetVelocity);
                    if (currentSpeed > maxSpeed) {
                        targetVelocity = targetVelocity * (maxSpeed / currentSpeed);
                    }
                    
                    b2Body_SetLinearVelocity(ball.bodyId, targetVelocity);
                    
                    // Log dragging for debugging (less frequently)
                    static int logCounter = 0;
                    if (++logCounter % 30 == 0) {
                        SDL_Log("Dragging ball %d: mouse=(%.2f,%.2f), ball=(%.2f,%.2f), force=(%.2f,%.2f)", 
                            draggedBallIndex, mousePhysicsPos.x, mousePhysicsPos.y, 
                            ballPos.x, ballPos.y, force.x, force.y);
                    }
                    
                    // Store current position for next frame
                    lastMousePos = mousePhysicsPos;
                }
            }
        }
        else {
            // Mouse released, stop dragging
            if (isDragging && draggedBallIndex >= 0 && draggedBallIndex < balls.size()) {
                balls[draggedBallIndex].isDragging = false;
                
                // Apply a small release velocity based on recent movement
                auto& ball = balls[draggedBallIndex];
                if (ball.isActive && !ball.isExploding) {
                    b2Vec2 currentVel = b2Body_GetLinearVelocity(ball.bodyId);
                    // Keep some of the current velocity for a natural release feel
                    b2Body_SetLinearVelocity(ball.bodyId, currentVel * 0.8f);
                }
                
                SDL_Log("Released ball %d", draggedBallIndex);
            }
            isDragging = false;
            draggedBallIndex = -1;
        }
    }

    // Draw a wall with appropriate color and damage effects based on hit count
    void drawWall(SDL_Renderer* renderer, const Wall& wall, float screenX, float screenY, 
                  float halfWidth, float halfHeight) const {
        // Calculate color based on hit count
        float hitRatio = static_cast<float>(wall.hitCount) / WALL_HIT_THRESHOLD;
        
        // Start with black and transition to yellow-orange-red as damage increases
        uint8_t red = 0, green = 0, blue = 0;
        
        if (wall.hitCount == 0) {
            // Undamaged wall - black
            red = green = blue = 0;
        } 
        else if (hitRatio < 0.33f) {
            // First stage - orange-ish with slight red
            red = 220;
            green = 120;
            blue = 0;
        }
        else if (hitRatio < 0.67f) {
            // Second stage - more red
            red = 240;
            green = 80;
            blue = 0;
        }
        else {
            // Final stage - bright red/yellow (near breaking)
            red = 255;
            green = static_cast<uint8_t>(hitRatio > 0.9f ? 255 : 40); // Flash yellow when about to break
            blue = 0;
        }
        
        SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
        
        // Draw the wall rectangle
        SDL_FRect rect = {
            screenX - halfWidth,
            screenY - halfHeight,
            halfWidth * 2,
            halfHeight * 2
        };
        
        SDL_RenderFillRect(renderer, &rect);
        
        // Add damage visual effects
        if (wall.hitCount > 0) {
            // Draw cracks that increase with damage
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180); // Whitish cracks
            
            int numCracks = 1 + static_cast<int>(hitRatio * 6);
            
            for (int i = 0; i < numCracks; i++) {
                // Generate random crack patterns
                float crackStartX = screenX - halfWidth * (0.8f * (static_cast<float>(rand()) / RAND_MAX));
                float crackStartY = screenY - halfHeight * (0.8f * (static_cast<float>(rand()) / RAND_MAX));
                
                // Create zigzag crack pattern
                float prevX = crackStartX;
                float prevY = crackStartY;
                
                int segments = 3 + static_cast<int>(hitRatio * 3);
                
                for (int j = 0; j < segments; j++) {
                    float nextX = screenX + halfWidth * (1.6f * (static_cast<float>(rand()) / RAND_MAX) - 0.8f);
                    float nextY = screenY + halfHeight * (1.6f * (static_cast<float>(rand()) / RAND_MAX) - 0.8f);
                    
                    SDL_RenderLine(renderer, prevX, prevY, nextX, nextY);
                    
                    prevX = nextX;
                    prevY = nextY;
                }
            }
            
            // Add pulsing effect when near breaking
            if (hitRatio > 0.75f) {
                static float pulseTimer = 0.0f;
                pulseTimer += 0.03f;
                
                float pulseAlpha = (sinf(pulseTimer * 10.0f) + 1.0f) * 0.5f * 150.0f; // Pulsing intensity
                
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, static_cast<uint8_t>(pulseAlpha));
                SDL_RenderRect(renderer, &rect);
                
                // Add a second rect for more intense effect when very close to breaking
                if (hitRatio > 0.9f) {
                    SDL_FRect innerRect = {
                        screenX - halfWidth + 2,
                        screenY - halfHeight + 2,
                        (halfWidth * 2) - 4,
                        (halfHeight * 2) - 4
                    };
                    SDL_RenderRect(renderer, &innerRect);
                }
            }
        }
    }
};

Physics::Physics(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<PhysicsImpl>(std::cref(title), std::cref(version), w, h)} {
}

Physics::~Physics() {
    auto&& g = this->m_impl;
    SDL_LockMutex(g->gameMtx);
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
    auto&& sdlHelper = this->m_impl->sdlHelper;
    string_view titleView = this->m_impl->title;
    sdlHelper.window = SDL_CreateWindow(titleView.data(), this->m_impl->INIT_WINDOW_W, this->m_impl->INIT_WINDOW_H, SDL_WINDOW_RESIZABLE);
    if (!sdlHelper.window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    sdlHelper.renderer = SDL_CreateRenderer(sdlHelper.window, nullptr);
    if (!sdlHelper.renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdlHelper.window);
        return false;
    }
    if (auto props = SDL_GetRendererProperties(sdlHelper.renderer); props != 0) {
        SDL_Log("Renderer created: %s\n", SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "default"));
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

    vector<SDL_Vertex> level;
    
    // Create a static persistent string to store the maze data
    static std::string persistentMazeStr;
    
    double previous = SDL_GetTicks();
    double accumulator = 0.0, currentTimeStep = 0.0;
    auto&& gState = this->m_impl->state;
    
    // Set the state to PLAY at startup
    gState = PhysicsImpl::States::PLAY;
    
    // Set a good default value for pixelsPerMeter
    this->m_impl->pixelsPerMeter = 20.0f;
    
    // Generate initial level at startup
    int display_w, display_h;
    SDL_GetWindowSize(window, &display_w, &display_h);
    this->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
    
    // Log physics world state after level generation
    SDL_Log("Physics world created, num walls: %zu, num balls: %zu", 
        this->m_impl->walls.size(), this->m_impl->balls.size());
    
    // Initial physics simulation step to ensure bodies are positioned
    if (B2_IS_NON_NULL(this->m_impl->physicsWorldId)) {
        SDL_Log("Performing initial physics step");
        b2World_Step(this->m_impl->physicsWorldId, this->m_impl->timeStep, 4);
    }
    
    while (gState != PhysicsImpl::States::DONE) {
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = SDL_GetTicks() - previous;
        previous = SDL_GetTicks();
        accumulator += elapsed;
        
        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP) {
            sdlHelper.do_events(ref(gState), ref(this->m_impl->camera));
            accumulator -= FIXED_TIME_STEP;
            currentTimeStep += FIXED_TIME_STEP;
        }

        // Update physics simulation if we're in PLAY state
        if (gState == PhysicsImpl::States::PLAY && 
            B2_IS_NON_NULL(this->m_impl->physicsWorldId)) {
             
            // Step Box2D world with sub-steps instead of velocity/position iterations
            b2World_Step(
                this->m_impl->physicsWorldId,
                this->m_impl->timeStep,
                4);  // Using 4 sub-steps as recommended in the migration guide
             
            // Handle collisions and physics interactions
            this->processPhysicsCollisions();
            this->updatePhysicsObjects();
        }

        // Audio stream updates
        if (SDL_GetAudioStreamAvailable(sdlHelper.audioStream) < sdlHelper.wavLength) {
            SDL_PutAudioStreamData(sdlHelper.audioStream, sdlHelper.wavBuffer, sdlHelper.wavLength);
            stream_bytes_len = SDL_GetAudioStreamAvailable(sdlHelper.audioStream);
        }
        
        // Get window dimensions
        SDL_GetWindowSize(window, &display_w, &display_h);
        
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light gray background
        SDL_RenderClear(renderer);
        
        // Generate new level if needed
        if (gState == PhysicsImpl::States::UPLOADING_LEVEL) {
            this->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
            gState = PhysicsImpl::States::PLAY;
        }
        
        // Draw the maze using the persistent maze string
        this->drawMaze(renderer, persistentMazeStr, display_w, display_h);
        
        // Draw the physics entities (balls, walls, exit) if we're in PLAY state
        if (gState == PhysicsImpl::States::PLAY && 
            B2_IS_NON_NULL(this->m_impl->physicsWorldId)) {
            this->drawPhysicsObjects(renderer);
        }
        
        // Draw debug test objects
        // this->drawDebugTestObjects(renderer);
        
        // Present the rendered frame
        SDL_RenderPresent(renderer);
        
        // FPS counter
        if (currentTimeStep >= 1000.0) {
            // Calculate frames per second
            SDL_Log("FPS: %d\n", static_cast<int>(1.0 / (elapsed / 1000.0)));
            // Calculate milliseconds per frame (correct formula)
            SDL_Log("Frame Time: %.3f ms/frame\n", elapsed);
            // Log physics world status
            SDL_Log("Walls: %zu, Balls: %zu", this->m_impl->walls.size(), this->m_impl->balls.size());
            currentTimeStep = 0.0;
        }
    }
    
    return true;
}

// Process collisions in the Box2D world
void Physics::processPhysicsCollisions() const {
    // Process contact events in Box2D 3.1.0 style
    b2ContactEvents contactEvents = b2World_GetContactEvents(this->m_impl->physicsWorldId);
    
    // Handle contact hit events
    for (int i = 0; i < contactEvents.hitCount; ++i) {
        b2ContactHitEvent* hitEvent = &contactEvents.hitEvents[i];
        b2BodyId bodyA = b2Shape_GetBody(hitEvent->shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(hitEvent->shapeIdB);
        
        // Process wall collisions
        this->m_impl->handleWallCollision(bodyA, bodyB);
        this->m_impl->handleWallCollision(bodyB, bodyA);
        
        // Process ball-to-ball collisions
        this->m_impl->handleBallCollision(bodyA, bodyB);
    }
    
    // Handle contact begin events
    for (int i = 0; i < contactEvents.beginCount; ++i) {
        b2ContactBeginTouchEvent* beginEvent = &contactEvents.beginEvents[i];
        b2BodyId bodyA = b2Shape_GetBody(beginEvent->shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(beginEvent->shapeIdB);
        
        // Check if a ball reached the exit
        uintptr_t exitId = 2000;  // Special ID for exit
        
        if (reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyA)) == exitId || 
            reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyB)) == exitId) {
            
            b2BodyId ballId = reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyA)) == exitId ? bodyB : bodyA;
            
            // Find the ball that reached the exit
            for (auto& ball : this->m_impl->balls) {
                if (b2Body_GetPosition(ball.bodyId).x == b2Body_GetPosition(ballId).x
                && b2Body_GetPosition(ball.bodyId).y == b2Body_GetPosition(ballId).y &&
                ball.isActive && !ball.isExploding) {
                    // Collect the ball
                    ball.isActive = false;
                    this->m_impl->exitCell.ballsCollected++;
                    this->m_impl->score += 100;
                    
                    // Remove the ball
                    b2DestroyBody(ball.bodyId);
                    break;
                }
            }
        }
    }
}

// Update physics objects state (balls, walls)
void Physics::updatePhysicsObjects() const {
    // Handle any destroyed walls 
    for (int i = this->m_impl->walls.size() - 1; i >= 0; --i) {
        auto& wall = this->m_impl->walls[i];
        if (wall.isDestroyed) {
            // Log destruction for debugging
            SDL_Log("Destroying wall %d with hit count %d", i, wall.hitCount);
            
            // Destroy the body in the physics world
            if (B2_IS_NON_NULL(wall.bodyId)) {
                b2DestroyBody(wall.bodyId);
                wall.bodyId = b2_nullBodyId;
            }
            
            // Remove from our tracking array
            this->m_impl->walls.erase(this->m_impl->walls.begin() + i);
        }
    }
    
    // Handle exploding balls
    for (int i = this->m_impl->balls.size() - 1; i >= 0; --i) {
        auto& ball = this->m_impl->balls[i];
        
        // Update explosion animation
        if (ball.isExploding) {
            ball.explosionTimer += this->m_impl->timeStep;
            
            if (ball.explosionTimer > 0.5f) { // After half a second, remove the ball
                if (B2_IS_NON_NULL(ball.bodyId)) {
                    b2DestroyBody(ball.bodyId);
                    ball.bodyId = b2_nullBodyId;
                }
                this->m_impl->balls.erase(this->m_impl->balls.begin() + i);
            }
        }
        
        // Check if ball is outside the play area
        if (ball.isActive && !ball.isExploding) {
            b2Vec2 position = b2Body_GetPosition(ball.bodyId);
            
            // Boundary check with revised bounds - the previous values were too restrictive
            float worldWidth = (this->m_impl->maxCols * this->m_impl->cellSize) / this->m_impl->pixelsPerMeter;
            float worldHeight = (this->m_impl->maxRows * this->m_impl->cellSize) / this->m_impl->pixelsPerMeter;
            
            // Use more reasonable bounds based on actual maze dimensions plus margin
            float margin = 10.0f;  // More forgiving margin
            if (position.x < -margin || position.x > worldWidth + margin || 
                position.y < -margin || position.y > worldHeight + margin) {
                
                SDL_Log("Ball %d marked inactive - out of bounds at (%.2f, %.2f)", i, position.x, position.y);
                ball.isActive = false;
            }
        }
    }
    
    // Handle ball dragging
    float mouseX, mouseY;
    uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);
    bool isMouseDown = mouseState & SDL_BUTTON_LMASK;
    this->m_impl->updateBallDrag(mouseX, mouseY, isMouseDown);
}

// Draw the physics objects (balls, walls, exit)
void Physics::drawPhysicsObjects(SDL_Renderer* renderer) const {
    // Get values from impl
    float cellSize = this->m_impl->cellSize;
    float offsetX = this->m_impl->offsetX;
    float offsetY = this->m_impl->offsetY;
    int display_w, display_h;
    SDL_GetCurrentRenderOutputSize(renderer, &display_w, &display_h);
    
    // Get camera for transformations
    const OrthographicCamera& camera = this->m_impl->camera;
    
    // Count active balls for debugging
    int activeBallCount = 0;
    for (const auto& ball : this->m_impl->balls) {
        if (ball.isActive) activeBallCount++;
    }
    
    SDL_Log("Drawing physics objects: walls=%zu, balls=%zu (active=%d), cellSize=%.2f, offsetX=%.2f, offsetY=%.2f, camera=(%.2f,%.2f,%.2f)",
        this->m_impl->walls.size(), this->m_impl->balls.size(), activeBallCount, cellSize, offsetX, offsetY,
        camera.x, camera.y, camera.zoom);
    
    // DEBUG: Draw a visible boundary around the physics world for debugging
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
    
    // Transform world bounds with camera
    SDL_FPoint topLeft = camera.worldToScreen(offsetX, offsetY, display_w, display_h);
    SDL_FPoint bottomRight = camera.worldToScreen(
        offsetX + this->m_impl->maxCols * cellSize, 
        offsetY + this->m_impl->maxRows * cellSize,
        display_w, display_h);
    
    SDL_FRect worldBounds = {
        topLeft.x, 
        topLeft.y, 
        bottomRight.x - topLeft.x,
        bottomRight.y - topLeft.y
    };
    SDL_RenderRect(renderer, &worldBounds);
    
    // Render walls based on their physical properties and current state
    for (const auto& wall : this->m_impl->walls) {
        if (wall.isDestroyed)
            continue;

        // Calculate world position
        float worldX = offsetX + (wall.col * cellSize);
        float worldY = offsetY + (wall.row * cellSize);
        
        // Transform with camera
        SDL_FPoint screenPos = camera.worldToScreen(worldX, worldY, display_w, display_h);
        float screenX = screenPos.x;
        float screenY = screenPos.y;
        
        // Scale dimensions based on zoom
        float halfWidth, halfHeight;
        
        // Determine wall dimensions based on wall type
        if (wall.type == '-') {
            halfWidth = cellSize * 0.5f * camera.zoom;
            halfHeight = cellSize * 0.1f * camera.zoom;
        } 
        else if (wall.type == '|') {
            halfWidth = cellSize * 0.1f * camera.zoom;
            halfHeight = cellSize * 0.5f * camera.zoom;
        }
        else { // '+' (junction/corner)
            halfWidth = halfHeight = cellSize * 0.15f * camera.zoom;
        }
        
        // Draw the wall with a color based on hit count
        this->m_impl->drawWall(renderer, wall, screenX, screenY, halfWidth, halfHeight);
    }
    
    // Render balls
    for (const auto& ball : this->m_impl->balls) {
        if (!ball.isActive) 
            continue;
            
        // Get ball position from Box2D
        b2Vec2 pos = b2Body_GetPosition(ball.bodyId);
        
        // Convert physics coordinates to world coordinates
        float worldX = offsetX + (pos.x * this->m_impl->pixelsPerMeter);
        float worldY = offsetY + (pos.y * this->m_impl->pixelsPerMeter);
        
        // Apply camera transform
        SDL_FPoint screenPos = camera.worldToScreen(worldX, worldY, display_w, display_h);
        float screenX = screenPos.x;
        float screenY = screenPos.y;
        
        // Scale radius based on zoom
        float radius = this->m_impl->BALL_RADIUS * this->m_impl->pixelsPerMeter * camera.zoom;
        
        // Debug log to verify ball positions

        // SDL_Log("Ball: physics(%.2f,%.2f) world(%.2f,%.2f) screen(%.2f,%.2f) r=%.2f active=%d", 
            // pos.x, pos.y, worldX, worldY, screenX, screenY, radius, ball.isActive ? 1 : 0);
        
        if (ball.isExploding) {
            // Render explosion animation
            float explosionProgress = ball.explosionTimer / 0.5f; // 0.5 is max time
            float expandedRadius = radius * (1.0f + explosionProgress * 2.0f);
            
            // Fade out as explosion progresses
            int alpha = static_cast<int>(255 * (1.0f - explosionProgress));
            
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, alpha); // Orange
            
            // Draw explosion as a circle with rays
            for (int w = 0; w < 16; w++) {
                float angle = static_cast<float>(w) * 3.14159f / 8.0f;
                SDL_RenderLine(
                    renderer,
                    screenX,
                    screenY,
                    screenX + cosf(angle) * expandedRadius,
                    screenY + sinf(angle) * expandedRadius
                );
            }
        }
        else {
            // Normal ball rendering - make it more visible with solid red circle
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Bright red
            
            // Use efficient circle rendering for large radius
            const int segments = 32; // Number of line segments to approximate the circle
            float previousX = screenX + radius;
            float previousY = screenY;
            
            for (int i = 1; i <= segments; i++) {
                float angle = (2.0f * SDL_PI_F * i) / segments;
                float x = screenX + radius * cosf(angle);
                float y = screenY + radius * sinf(angle);
                
                SDL_RenderLine(renderer, previousX, previousY, x, y);
                previousX = x;
                previousY = y;
            }
            
            // Fill the circle efficiently
            for (int y = -radius; y <= radius; y += 1) {
                float width = sqrtf(radius * radius - y * y);
                SDL_RenderLine(renderer, screenX - width, screenY + y, screenX + width, screenY + y);
            }
            
            // Add highlight effect for better visualization
            SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255); // Light red highlight
            float highlight_radius = radius * 0.5f;
            float highlightOffsetX = -radius * 0.2f; // Offset to upper left for light effect
            float highlightOffsetY = -radius * 0.2f;
            
            for (int y = -highlight_radius; y <= 0; y += 1) {
                float width = sqrtf(highlight_radius * highlight_radius - y * y);
                SDL_RenderLine(renderer, 
                    screenX + highlightOffsetX - width/2, 
                    screenY + highlightOffsetY + y, 
                    screenX + highlightOffsetX + width/2, 
                    screenY + highlightOffsetY + y);
            }
        }
    }
    
    // Render exit cell
    if (B2_IS_NON_NULL(this->m_impl->exitCell.bodyId)) {
        // Calculate world position
        float worldX = offsetX + (this->m_impl->exitCell.col * cellSize);
        float worldY = offsetY + (this->m_impl->exitCell.row * cellSize);
        
        // Apply camera transform
        SDL_FPoint screenPos = camera.worldToScreen(worldX, worldY, display_w, display_h);
        float screenX = screenPos.x;
        float screenY = screenPos.y;
        
        // Scale radius based on zoom
        float radius = cellSize * 0.4f * camera.zoom;
        
        // Use a bright color for the exit
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Bright green
        
        // Fill the exit circle
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderPoint(renderer, screenX + x, screenY + y);
                }
            }
        }
        
        // Draw X inside the exit with a contrasting color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
        SDL_RenderLine(renderer, 
            screenX - radius/2, screenY - radius/2,
            screenX + radius/2, screenY + radius/2);
        SDL_RenderLine(renderer, 
            screenX + radius/2, screenY - radius/2,
            screenX - radius/2, screenY + radius/2);
            
        // Log exit position
        // SDL_Log("Exit at screen pos (%.2f, %.2f)", screenX, screenY);
    }
}

// Draw the maze background
void Physics::drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int display_w, int display_h) const {
    if (cells.empty()) {
        SDL_Log("ERROR: Empty maze data provided for drawing.\n");
        return;
    }
    
    // Get camera state for transformations
    const OrthographicCamera& camera = this->m_impl->camera;
    
    // Calculate maze dimensions
    int maxCols = 0;
    int maxRows = 0;
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
    
    // Reset for drawing
    currentRow = 0;
    int currentCol = 0;
    
    // Calculate cell size to fit the display with proper padding
    float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols);
    float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows);
    float cellSize = std::min(cellW, cellH) * 0.95f; // 5% padding for better visibility
    
    // Make sure path cells are large enough to fit balls
    float minCellSize = 20.0f; // Minimum size for comfortable navigation
    cellSize = std::max(cellSize, minCellSize);
    
    // Center the maze in the display
    float mazeWidth = maxCols * cellSize;
    float mazeHeight = maxRows * cellSize;
    float offsetX = (display_w - mazeWidth) / 2.0f;
    float offsetY = (display_h - mazeHeight) / 2.0f;
    
    // Ensure the maze is centered even if it's bigger than the display
    offsetX = std::max(0.0f, offsetX);
    offsetY = std::max(0.0f, offsetY);
    
    // Store these values in the implementation for physics objects to use
    this->m_impl->cellSize = cellSize;
    this->m_impl->offsetX = offsetX;
    this->m_impl->offsetY = offsetY;
    this->m_impl->maxCols = maxCols;
    this->m_impl->maxRows = maxRows;
    
    // Update the pixels per meter ratio for physics calculations
    this->m_impl->pixelsPerMeter = cellSize;
    
    // Draw maze background
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light background
    SDL_RenderClear(renderer);
    
    // Draw maze path cells with different colors for the grid
    for (size_t i = 0; i < mazeLen; i++) {
        char c = mazeData[i];
        if (c == '\n') {
            currentCol = 0;
            currentRow++;
            continue;
        }
        
        // Calculate base cell position
        float baseX = offsetX + (currentCol * cellSize);
        float baseY = offsetY + (currentRow * cellSize);
        
        // Apply camera transformation
        SDL_FPoint screenPos = camera.worldToScreen(baseX, baseY, display_w, display_h);
        float x = screenPos.x;
        float y = screenPos.y;
        
        // Scale the cell size according to zoom
        float scaledCellSize = cellSize * camera.zoom;
        
        if (c == ' ') { 
            // Path cell - light gray
            if (currentRow % 2 == 1 && currentCol % 2 == 1) {
                // Make path cells stand out more
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // Lighter gray
                
                // We need to draw a rotated rectangle when the camera is rotated
                if (camera.rotation != 0.0f) {
                    // Calculate the four corners of the rotated rectangle
                    float halfSize = scaledCellSize * 0.5f;
                    float cos_r = cosf(camera.rotation);
                    float sin_r = sinf(camera.rotation);
                    
                    SDL_Vertex vertices[6];
                    // Define the four corner points of the rotated rectangle
                    SDL_FPoint points[4] = {
                        {x - halfSize, y - halfSize},  // Top left
                        {x + halfSize, y - halfSize},  // Top right
                        {x + halfSize, y + halfSize},  // Bottom right
                        {x - halfSize, y + halfSize}   // Bottom left
                    };
                    
                    // Create a color for the vertices
                    SDL_FColor color = {220.0f/255.0f, 220.0f/255.0f, 220.0f/255.0f, 1.0f};
                    
                    // Create the vertices for two triangles (forming the rectangle)
                    for (int i = 0; i < 4; i++) {
                        vertices[i].position = points[i];
                        vertices[i].color = color;
                        vertices[i].tex_coord = {0.0f, 0.0f};  // We're not using textures
                    }
                    
                    // Create two triangles from the vertices (0,1,3) and (1,2,3)
                    vertices[4] = vertices[3];  // Duplicate vertex 3
                    vertices[5] = vertices[1];  // Duplicate vertex 1
                    
                    // Render the triangles
                    SDL_RenderGeometry(renderer, nullptr, vertices, 6, nullptr, 0);
                }
                else {
                    // No rotation, just draw a regular rectangle
                    SDL_FRect rect = {x - scaledCellSize * 0.5f, y - scaledCellSize * 0.5f, 
                                    scaledCellSize, scaledCellSize};
                    SDL_RenderFillRect(renderer, &rect);
                    
                    // Add a subtle grid pattern to help with depth perception
                    SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
                    SDL_RenderRect(renderer, &rect);
                }
            }
        }
        
        currentCol++;
    }
}

// Generate a new level
void Physics::generateNewLevel(std::string& persistentMazeStr, int display_w, int display_h) const {
    auto m_ptr = mazes::factory::create_q(INIT_MAZE_ROWS, INIT_MAZE_COLS);
    if (!m_ptr.has_value()) {
    
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze: %s\n");
        return;
    }

    auto s = mazes::stringz::stringify(cref(m_ptr.value()));

    persistentMazeStr = s;
    
    // Calculate cell size
    int maxCols = 11; // Hardcoded maze width
    int maxRows = 11; // Hardcoded maze height
    float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols);
    float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows);
    float cellSize = std::min(cellW, cellH);
    
    // Create physics objects for the maze
    this->m_impl->createMazePhysics(persistentMazeStr, cellSize);
    
    SDL_Log("New level generated successfully");
}

// Draw debug test objects to verify rendering is working
void Physics::drawDebugTestObjects(SDL_Renderer* renderer) const {
    int display_w, display_h;
    SDL_GetCurrentRenderOutputSize(renderer, &display_w, &display_h);
    
    // Draw a green rectangle to confirm we can render anything
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect greenRect = {20.0f, 20.0f, 100.0f, 100.0f};
    SDL_RenderFillRect(renderer, &greenRect);
    
    // Draw a red circle
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    float centerX = display_w / 4.0f;
    float centerY = display_h / 4.0f;
    float radius = 40.0f;
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
    
    // Draw a blue cross
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    centerX = display_w * 3.0f / 4.0f;
    centerY = display_h * 3.0f / 4.0f;
    SDL_RenderLine(renderer, centerX - 50.0f, centerY, centerX + 50.0f, centerY);
    SDL_RenderLine(renderer, centerX, centerY - 50.0f, centerX, centerY + 50.0f);
    
    // SDL_Log("Debug shapes drawn at (%d, %d)", display_w, display_h);
}

// Camera coordinate transformation implementation
SDL_FPoint OrthographicCamera::worldToScreen(float worldX, float worldY, int screenWidth, int screenHeight) const {
    // Step 1: Apply camera position offset
    // Note: We ADD the camera position to move in the opposite direction
    // This creates the illusion of camera movement when we're actually moving the world
    float offsetX = worldX + this->x;
    float offsetY = worldY + this->y;
    
    // Step 2: Apply zoom factor - centered at screen center
    // Move to origin, scale, then move back
    float screenCenterX = screenWidth / 2.0f;
    float screenCenterY = screenHeight / 2.0f;
    
    float zoomOffsetX = offsetX - screenCenterX;
    float zoomOffsetY = offsetY - screenCenterY;
    
    float zoomedX = screenCenterX + zoomOffsetX * this->zoom;
    float zoomedY = screenCenterY + zoomOffsetY * this->zoom;
    
    // Step 3: Apply rotation around screen center if needed
    float finalX = zoomedX;
    float finalY = zoomedY;
    
    if (this->rotation != 0.0f) {
        float cosR = cosf(this->rotation);
        float sinR = sinf(this->rotation);
        
        // Translate to origin, rotate, translate back
        float rotOffsetX = zoomedX - screenCenterX;
        float rotOffsetY = zoomedY - screenCenterY;
        
        finalX = screenCenterX + rotOffsetX * cosR - rotOffsetY * sinR;
        finalY = screenCenterY + rotOffsetX * sinR + rotOffsetY * cosR;
    }
    
    return {finalX, finalY};
}

void OrthographicCamera::screenToWorld(float screenX, float screenY, float& worldX, float& worldY, int screenWidth, int screenHeight) const {
    float screenCenterX = screenWidth / 2.0f;
    float screenCenterY = screenHeight / 2.0f;
    
    // Step 1: Undo rotation
    float unrotatedX = screenX;
    float unrotatedY = screenY;
    
    if (this->rotation != 0.0f) {
        float cosR = cosf(-this->rotation);  // Use negative rotation to reverse
        float sinR = sinf(-this->rotation);
        
        float rotOffsetX = screenX - screenCenterX;
        float rotOffsetY = screenY - screenCenterY;
        
        unrotatedX = screenCenterX + rotOffsetX * cosR - rotOffsetY * sinR;
        unrotatedY = screenCenterY + rotOffsetX * sinR + rotOffsetY * cosR;
    }
    
    // Step 2: Undo zoom (centered at screen center)
    float zoomOffsetX = unrotatedX - screenCenterX;
    float zoomOffsetY = unrotatedY - screenCenterY;
    
    float unzoomedX = screenCenterX + zoomOffsetX / this->zoom;
    float unzoomedY = screenCenterY + zoomOffsetY / this->zoom;
    
    // Step 3: Undo camera position offset
    // Note: We SUBTRACT the camera position (opposite of worldToScreen)
    worldX = unzoomedX - this->x;
    worldY = unzoomedY - this->y;
}

// Handle camera input (keyboard, mouse)
void Physics::handleCameraInput() const {
    // Get SDL keyboard state
    const auto* keyState = SDL_GetKeyboardState(nullptr);
    
    // Camera movement with WASD keys
    if (keyState[SDL_SCANCODE_W]) {
        this->m_impl->camera.y -= this->m_impl->camera.panSpeed;
    }
    if (keyState[SDL_SCANCODE_S]) {
        this->m_impl->camera.y += this->m_impl->camera.panSpeed;
    }
    if (keyState[SDL_SCANCODE_A]) {
        this->m_impl->camera.x -= this->m_impl->camera.panSpeed;
    }
    if (keyState[SDL_SCANCODE_D]) {
        this->m_impl->camera.x += this->m_impl->camera.panSpeed;
    }
    
    // Camera rotation with Q and E keys
    if (keyState[SDL_SCANCODE_Q]) {
        this->m_impl->camera.rotation -= this->m_impl->camera.rotationSpeed;
    }
    if (keyState[SDL_SCANCODE_E]) {
        this->m_impl->camera.rotation += this->m_impl->camera.rotationSpeed;
    }
    
    // Camera zoom with + and - keys
    if (keyState[SDL_SCANCODE_EQUALS]) { // + key
        this->m_impl->camera.zoom *= (1.0f + this->m_impl->camera.zoomSpeed);
    }
    if (keyState[SDL_SCANCODE_MINUS]) { // - key
        this->m_impl->camera.zoom /= (1.0f + this->m_impl->camera.zoomSpeed);
    }
    
    // Reset camera with R key
    if (keyState[SDL_SCANCODE_R]) {
        this->m_impl->camera.x = 0.0f;
        this->m_impl->camera.y = 0.0f;
        this->m_impl->camera.zoom = 1.0f;
        this->m_impl->camera.rotation = 0.0f;
    }
    
    // Ensure zoom stays within reasonable limits
    this->m_impl->camera.zoom = std::max(0.1f, std::min(5.0f, this->m_impl->camera.zoom));
}

// Update camera position and properties
void Physics::updateCamera(float deltaTime) const {
    // Handle user input for camera control
    handleCameraInput();
}
