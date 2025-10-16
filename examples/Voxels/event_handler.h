#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "craft_types.h"

struct InputState {
    bool flying;
    int item_index;
    float mouse_sensitivity;
    float movement_speed;
};

struct PlayerState {
    float x, y, z;
    float rx, ry;
    float t;
};

#include <vector>

struct SDL_Window;

class EventHandler {
public:
    static bool handleEventsAndMotion(double dt, bool& window_resizes, 
                                     SDL_Window* window, PlayerState& player_state,
                                     InputState& input_state, bool capture_mouse);
    
    static void handleLeftClick(const PlayerState& player_state, 
                               std::vector<CraftChunk>& chunks, int chunk_size, int item_index);
    static void handleRightClick(const PlayerState& player_state,
                                std::vector<CraftChunk>& chunks, int chunk_size, int item_index);
    static void handleMiddleClick(const PlayerState& player_state,
                                 std::vector<CraftChunk>& chunks, int chunk_size, int& item_index);
    static void handleLightToggle(const PlayerState& player_state,
                                 std::vector<CraftChunk>& chunks, int chunk_size);
};

#endif // EVENT_HANDLER_H
