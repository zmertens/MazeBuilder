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

#include <box2d/box2d.h>

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
    
    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f; // Number of hits before a wall breaks
    static constexpr float BALL_RADIUS = 0.2f;
    static constexpr float WALL_WIDTH = 0.1f;
    static constexpr int MAX_BALLS = 10;
    
    struct Wall {
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        int hitCount = 0;
        bool isDestroyed = false;
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
    std::deque<WorkItem> workQueue;
    std::vector<SDL_Thread*> threads;
    SDL_Mutex* gameMtx;
    SDL_Condition* gameCond;
    SDLTexture entityTexture;
    int pendingWorkCount;
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
        for (size_t i = 0; i < segment.size(); i++) {
            char c = segment[i];
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            float scale = 10.0f;
            float viewScaleX = static_cast<float>(INIT_WINDOW_W) / (static_cast<float>(columnsInMaze) * scale);
            float viewScaleY = static_cast<float>(INIT_WINDOW_H) / (static_cast<float>(rowsInMaze) * scale / 2.0f);
            float viewScale = std::min(viewScaleX, viewScaleY) * 0.9f;
            float x = static_cast<float>(currentCol) * scale * viewScale;
            float y = static_cast<float>(currentRow) * scale / 2.0f * viewScale;
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
                float x = static_cast<float>(currentCol) * cellSize.x;
                float y = static_cast<float>((currentRow - 1) / 2) * cellSize.y;
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
        float lengthUnitsPerMeter = 128.0f;
        b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);
    
        b2WorldDef worldDef = b2DefaultWorldDef();
    
        // Realistic gravity is achieved by multiplying gravity by the length unit.
        worldDef.gravity.y = 9.8f * lengthUnitsPerMeter;

        physicsWorldId = b2CreateWorld(&worldDef);
        
        // Clear any existing entities
        walls.clear();
        balls.clear();
        
        SDL_Log("Box2D physics world initialized");
    }
    
    // Convert screen coordinates to physics world coordinates
    b2Vec2 screenToPhysics(float screenX, float screenY) {
        return {screenX / pixelsPerMeter, screenY / pixelsPerMeter};
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
            (float)((rand() % 100) - 50) / 25.0f,  // Random x velocity
            (float)((rand() % 100) - 50) / 25.0f   // Random y velocity
        };

        // bodyDef.bullet = true;  // Enable continuous collision detection
        // bodyDef.userData.pointer = reinterpret_cast<uintptr_t>(this);
        
        b2BodyId ballBodyId = b2CreateBody(physicsWorldId, &bodyDef);
        
        // Create a circle shape for the ball
        b2ShapeDef circleDef = b2DefaultShapeDef();
        circleDef.density = 1.0f;
        
        b2Circle circle;
        circle.radius = BALL_RADIUS;
        b2ShapeId ballShapeId = b2CreateCircleShape(ballBodyId, &circleDef, &circle);
        
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
        
        // Reset tracking variables
        currentRow = 0;
        int currentCol = 0;
        
        // Create world boundaries
        float worldWidth = maxCols * cellSize / pixelsPerMeter;
        float worldHeight = maxRows * cellSize / pixelsPerMeter;
        
        // Create walls for maze
        for (size_t i = 0; i < mazeLen; i++) {
            char c = mazeData[i];
            
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Convert to physics coordinates
            float x = (currentCol * cellSize) / pixelsPerMeter;
            float y = (currentRow * cellSize) / pixelsPerMeter;
            
            // Create walls for different maze characters
            if (c == '-' || c == '|') {
                b2BodyDef wallDef = b2DefaultBodyDef();
                wallDef.type = b2_staticBody;
                wallDef.position = {x, y};
                
                // Store the wall index in the user data to identify it later
                int wallIndex = walls.size();
                uintptr_t wallId = 1000 + wallIndex; // Use offset to identify as wall
                // wallDef.userData.pointer = wallId;
                
                b2BodyId wallBodyId = b2CreateBody(physicsWorldId, &wallDef);
                
                // Create shape definition
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = 0.0f;  // static bodies
                
                // Create different shapes based on wall orientation
                if (c == '-') {
                    // Horizontal wall
                    float halfWidth = cellSize / (2.0f * pixelsPerMeter);
                    float halfHeight = WALL_WIDTH / 2.0f;
                    
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2ShapeId wallShapeId = b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    Wall wall;
                    wall.bodyId = wallBodyId;
                    wall.shapeId = wallShapeId;
                    wall.hitCount = 0;
                    wall.isDestroyed = false;
                    walls.push_back(wall);
                } else {
                    // Vertical wall
                    float halfWidth = WALL_WIDTH / 2.0f;
                    float halfHeight = cellSize / (2.0f * pixelsPerMeter);
                    
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2ShapeId wallShapeId = b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    Wall wall;
                    wall.bodyId = wallBodyId;
                    wall.shapeId = wallShapeId;
                    wall.hitCount = 0;
                    wall.isDestroyed = false;
                    walls.push_back(wall);
                }
            }
            
            currentCol++;
        }
        
        // Create a random exit cell
        int exitRow = rand() % maxRows;
        int exitCol = rand() % maxCols;
        
        exitCell.row = exitRow;
        exitCell.col = exitCol;
        
        b2BodyDef exitDef = b2DefaultBodyDef();
        exitDef.type = b2_staticBody;
        exitDef.position = {
            (exitCol * cellSize) / pixelsPerMeter,
            (exitRow * cellSize) / pixelsPerMeter
        };
        uintptr_t exitId = 2000; // Use offset to identify as exit
        // exitDef.userData.pointer = exitId;
        
        exitCell.bodyId = b2CreateBody(physicsWorldId, &exitDef);
        
        // b2Circle circleDef = b2CreateCircleShape();
        // circleDef.density = 0.0f;  // static body
        
        b2Circle exitCircle;
        exitCircle.radius = cellSize / (2.0f * pixelsPerMeter);
        // exitCell.shapeId = b2CreateCircleShape(exitCell.bodyId, &circleDef, &exitCircle);
        
        // Create initial balls
        int numInitialBalls = 5;
        for (int i = 0; i < numInitialBalls; i++) {
            // Pick random cell that's not the exit
            int ballRow, ballCol;
            do {
                ballRow = rand() % maxRows;
                ballCol = rand() % maxCols;
            } while (ballRow == exitRow && ballCol == exitCol);
            
            float ballX = (ballCol * cellSize + cellSize / 2) / pixelsPerMeter;
            float ballY = (ballRow * cellSize + cellSize / 2) / pixelsPerMeter;
            
            balls.push_back(createBall(ballX, ballY));
        }
        
        SDL_Log("Maze physics created with %zu walls and %zu balls", walls.size(), balls.size());
    }
    
    // Utility method for handling wall collisions
    void handleWallCollision(b2BodyId possibleWallId, b2BodyId possibleBallId) {
        // If this is a ball hitting a wall
        auto* wallUserData = b2Body_GetUserData(possibleWallId);
        
        // Check if it's a wall by checking the user data range
        // Walls are stored with pointer values starting at 1000
        // if (wallUserData >= 1000 && wallUserData.pointer < 2000) {
        //     int wallIndex = wallUserData.pointer - 1000;
            
        //     // Make sure the index is valid
        //     if (wallIndex >= 0 && wallIndex < walls.size()) {
        //         Wall& wall = walls[wallIndex];
        //         wall.hitCount++;
                
        //         // Check if wall should break
        //         if (wall.hitCount >= WALL_HIT_THRESHOLD && !wall.isDestroyed) {
        //             wall.isDestroyed = true;
        //             // Schedule this wall for removal (can't remove during collision callback)
        //         }
        //     }
        // }
    }
    
    // Utility method for handling ball-to-ball collisions
    void handleBallCollision(b2BodyId bodyAId, b2BodyId bodyBId) {
        // Check if both are balls (balls have user data pointer == this)
        auto* userDataA = b2Body_GetUserData(bodyAId);
        auto* userDataB = b2Body_GetUserData(bodyBId);
        
        if (reinterpret_cast<uintptr_t>(userDataA) == reinterpret_cast<uintptr_t>(this) && 
        reinterpret_cast<uintptr_t>(userDataB) == reinterpret_cast<uintptr_t>(this)) {
            
            // Find the corresponding ball objects
            Ball* ballA = nullptr;
            Ball* ballB = nullptr;
            
            for (auto& ball : balls) {
                if (b2Body_GetPosition(ball.bodyId).x == b2Body_GetPosition(bodyAId).x || b2Body_GetPosition(ball.bodyId).y == b2Body_GetPosition(bodyAId).y) ballA = &ball;
                if (b2Body_GetPosition(ball.bodyId).x == b2Body_GetPosition(bodyBId).x || b2Body_GetPosition(ball.bodyId).y == b2Body_GetPosition(bodyBId).y) ballB = &ball;
            }
            
            if (ballA && ballB) {
                // Start explosion animation for both balls
                ballA->isExploding = true;
                ballB->isExploding = true;
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
    
    // Audio setup
    // ...existing code...
    
    auto&& renderer = sdlHelper.renderer;
    SDL_SetRenderVSync(renderer, true);
    auto&& window = sdlHelper.window;

    vector<SDL_Vertex> level;
    string_view cells;
    double previous = SDL_GetTicks();
    double accumulator = 0.0, currentTimeStep = 0.0;
    auto&& gState = this->m_impl->state;
    
    // Set a good default value for pixelsPerMeter
    this->m_impl->pixelsPerMeter = 20.0f;
    
    while (gState != PhysicsImpl::States::DONE) {
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = SDL_GetTicks() - previous;
        previous = SDL_GetTicks();
        accumulator += elapsed;
        
        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP) {
            sdlHelper.do_events(ref(gState));
            
            // Update physics simulation if we're in PLAY state
            if (gState == PhysicsImpl::States::PLAY && 
               B2_IS_NON_NULL(this->m_impl->physicsWorldId)) {
                
                // Step Box2D world
                b2World_Step(
                    this->m_impl->physicsWorldId,
                    this->m_impl->timeStep,
                    this->m_impl->velocityIterations);
                
                // Handle collisions and physics interactions
                this->processPhysicsCollisions();
                this->updatePhysicsObjects();
            }
            
            accumulator -= FIXED_TIME_STEP;
            currentTimeStep += FIXED_TIME_STEP;
        }
        
        // Get window dimensions
        int display_w, display_h;
        SDL_GetWindowSize(window, &display_w, &display_h);
        
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        // Generate new level if needed
        if (gState == PhysicsImpl::States::UPLOADING_LEVEL) {
            this->generateNewLevel(display_w, display_h);
            gState = PhysicsImpl::States::PLAY;
        }
        
        // Draw the maze
        this->drawMaze(renderer, cells, display_w, display_h);
        
        // Draw the physics entities (balls, walls, exit) if we're in PLAY state
        if (gState == PhysicsImpl::States::PLAY && 
            B2_IS_NON_NULL(this->m_impl->physicsWorldId)) {
            this->drawPhysicsObjects(renderer);
        }
        
        // Present the rendered frame
        SDL_RenderPresent(renderer);
        
        // FPS counter
        if (currentTimeStep >= 1.0) {
            SDL_Log("FPS: %d\n", static_cast<int>(1.0 / (elapsed / 1000.0)));
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
    
    // Process contact begin events
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
            b2DestroyBody(wall.bodyId);
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
                b2DestroyBody(ball.bodyId);
                this->m_impl->balls.erase(this->m_impl->balls.begin() + i);
            }
        }
        
        // Check if ball is outside the play area
        if (ball.isActive && !ball.isExploding) {
            b2Vec2 position = b2Body_GetPosition(ball.bodyId);
            if (position.x < -5 || position.x > 100 + 5 || 
                position.y < -5 || position.y > 100 + 5) {
                
                // Ball is outside the maze, mark it as inactive
                ball.isActive = false;
            }
        }
    }
    
    // Handle ball dragging
    float mouseX, mouseY;
    uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);
    b2Vec2 mousePos = this->m_impl->screenToPhysics(mouseX, mouseY);
    
    // Start dragging a ball if mouse button is pressed
    if (mouseState & SDL_BUTTON_LMASK) {
        if (!this->m_impl->isDragging) {
            // Check if we're clicking on a ball
            for (int i = 0; i < this->m_impl->balls.size(); i++) {
                auto& ball = this->m_impl->balls[i];
                
                if (!ball.isActive || ball.isExploding)
                    continue;
                    
                b2Vec2 ballPos = b2Body_GetPosition(ball.bodyId);
                float distance = b2Distance(ballPos, mousePos);
                
                if (distance <= this->m_impl->BALL_RADIUS * 1.5f) {
                    this->m_impl->isDragging = true;
                    this->m_impl->draggedBallIndex = i;
                    this->m_impl->lastMousePos = mousePos;
                    break;
                }
            }
        } 
        else if (this->m_impl->draggedBallIndex >= 0 && 
                this->m_impl->draggedBallIndex < this->m_impl->balls.size()) {
            // Apply force to the ball based on mouse movement
            auto& ball = this->m_impl->balls[this->m_impl->draggedBallIndex];
            
            if (ball.isActive && !ball.isExploding) {
                b2Vec2 delta = mousePos - this->m_impl->lastMousePos;
                b2Body_ApplyForceToCenter(ball.bodyId, delta * 50.0f, true); // Scale the force
                this->m_impl->lastMousePos = mousePos;
            }
        }
    } 
    else {
        // Mouse released, stop dragging
        this->m_impl->isDragging = false;
        this->m_impl->draggedBallIndex = -1;
    }
}

// Draw the physics objects (balls, walls, exit)
void Physics::drawPhysicsObjects(SDL_Renderer* renderer) const {
    // Render walls
    for (const auto& wall : this->m_impl->walls) {
        // Get wall position and dimensions
        b2Vec2 pos = b2Body_GetPosition(wall.bodyId);
        SDL_FPoint screenPos = this->m_impl->physicsToScreen(pos.x, pos.y);
        
        // Calculate wall dimensions based on shape type
        float halfWidth = 10.0f;  // Default size
        float halfHeight = 10.0f;
        
        // Adjust size based on wall orientation (horizontal or vertical)
        // This is an approximation since we can't easily get the shape data in Box2D 3.1.0
        halfWidth = 20.0f;
        halfHeight = 5.0f;
        
        SDL_FRect rect = {
            screenPos.x - halfWidth, 
            screenPos.y - halfHeight,
            halfWidth * 2,
            halfHeight * 2
        };
        
        // Color walls based on hit count
        int hitRatio = (int)(255 * wall.hitCount / this->m_impl->WALL_HIT_THRESHOLD);
        SDL_SetRenderDrawColor(renderer, 0, 0, 255 - hitRatio, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
    
    // Render balls
    for (const auto& ball : this->m_impl->balls) {
        if (!ball.isActive) 
            continue;
            
        b2Vec2 pos = b2Body_GetPosition(ball.bodyId);
        SDL_FPoint screenPos = this->m_impl->physicsToScreen(pos.x, pos.y);
        float radius = this->m_impl->BALL_RADIUS * this->m_impl->pixelsPerMeter;
        
        if (ball.isExploding) {
            // Render explosion animation
            float explosionProgress = ball.explosionTimer / 0.5f; // 0.5 is max time
            float expandedRadius = radius * (1.0f + explosionProgress * 2.0f);
            
            // Fade out as explosion progresses
            int alpha = (int)(255 * (1.0f - explosionProgress));
            
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, alpha); // Orange
            
            // Draw explosion as a circle
            for (int w = 0; w < 8; w++) {
                float angle = (float)w * 3.14159f / 4.0f;
                SDL_RenderLine(
                    renderer,
                    screenPos.x,
                    screenPos.y,
                    screenPos.x + cosf(angle) * expandedRadius,
                    screenPos.y + sinf(angle) * expandedRadius
                );
            }
        }
        else {
            // Normal ball rendering
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            
            // Draw ball as a filled circle
            for (int y = -radius; y <= radius; y++) {
                for (int x = -radius; x <= radius; x++) {
                    if (x*x + y*y <= radius*radius) {
                        SDL_RenderPoint(renderer, screenPos.x + x, screenPos.y + y);
                    }
                }
            }
        }
    }
    
    // Render exit cell
    if (B2_IS_NON_NULL(this->m_impl->exitCell.bodyId)) {
        b2Vec2 pos = b2Body_GetPosition(this->m_impl->exitCell.bodyId);
        SDL_FPoint screenPos = this->m_impl->physicsToScreen(pos.x, pos.y);
        float radius = this->m_impl->BALL_RADIUS * 2 * this->m_impl->pixelsPerMeter;
        
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        
        // Draw exit as a circle with X inside
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderPoint(renderer, screenPos.x + x, screenPos.y + y);
                }
            }
        }
        
        // Draw X
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderLine(renderer, 
            screenPos.x - radius/2, screenPos.y - radius/2,
            screenPos.x + radius/2, screenPos.y + radius/2);
        SDL_RenderLine(renderer, 
            screenPos.x + radius/2, screenPos.y - radius/2,
            screenPos.x - radius/2, screenPos.y + radius/2);
    }
    
    // Draw score
    char scoreText[32];
    SDL_snprintf(scoreText, sizeof(scoreText), "Score: %d", this->m_impl->exitCell.ballsCollected);
    // We would need a text rendering function here
}

// Draw the maze background
void Physics::drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int display_w, int display_h) const {
    if (cells.empty()) {
        return;
    }
    
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
    
    // Calculate cell size and offsets
    float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols + 1);
    float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows + 1);
    float cellSize = std::min(cellW, cellH);
    float offsetX = (display_w - (maxCols * cellSize)) / 2.0f;
    float offsetY = (display_h - (maxRows * cellSize)) / 2.0f;
    
    // Store cell size for physics calculations
    float storedCellSize = cellSize;

    // Draw maze cells
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light gray background
    for (size_t i = 0; i < mazeLen; i++) {
        char c = mazeData[i];
        if (c == '\n') {
            currentCol = 0;
            currentRow++;
            continue;
        }
        
        float x = offsetX + (currentCol * cellSize);
        float y = offsetY + (currentRow * cellSize);
        
        if (c == ' ' && currentRow % 2 == 1) {
            // Path cell
            SDL_FRect rect = {x, y, cellSize, cellSize};
            SDL_RenderFillRect(renderer, &rect);
        }
        
        currentCol++;
    }
}

// Generate a new level
void Physics::generateNewLevel(int display_w, int display_h) const {
    static constexpr auto INIT_MAZE_ROWS = 25, INIT_MAZE_COLS = 25;
    auto maze_ptr = mazes::factory::create_q(INIT_MAZE_ROWS, INIT_MAZE_COLS);
    
    if (!maze_ptr.has_value()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze with rows: %d and cols: %d\n", INIT_MAZE_ROWS, INIT_MAZE_COLS);
        return;
    }
    
    SDL_Log("New level uploading with rows: %d and cols: %d\n", INIT_MAZE_ROWS, INIT_MAZE_COLS);
    std::string mazeStr = mazes::stringz::stringify(maze_ptr.value());
    static std::string persistentMazeStr;
    persistentMazeStr = mazeStr;
    
    // Calculate cell size
    int maxCols = INIT_MAZE_COLS * 2 + 1;
    int maxRows = INIT_MAZE_ROWS * 2 + 1;
    float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols);
    float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows);
    float cellSize = std::min(cellW, cellH);
    
    // Create physics objects for the maze
    this->m_impl->createMazePhysics(persistentMazeStr, cellSize);
    
    SDL_Log("New level generated successfully");
}
