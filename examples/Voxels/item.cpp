#include "item.h"

#include <cmath>
#include <SDL3/SDL_stdinc.h>

std::array<int, item::TOTAL_ITEMS> item::items = {
    // items the user can build
    GRASS,
    SAND,
    STONE,
    BRICK,
    WOOD,
    CEMENT,
    DIRT,
    PLANK,
    SNOW,
    GLASS,
    COBBLE,
    LIGHT_STONE,
    DARK_STONE,
    CHEST,
    LEAVES,
    TALL_GRASS,
    YELLOW_FLOWER,
    RED_FLOWER,
    PURPLE_FLOWER,
    SUN_FLOWER,
    WHITE_FLOWER,
    BLUE_FLOWER,
    SDL_LOGO,
    SFML_LOGO,
    CACTUS_1,
    CACTUS_2,
    COLOR_00,
    COLOR_01,
    COLOR_02,
    COLOR_03,
    COLOR_04,
    COLOR_05,
    COLOR_06,
    COLOR_07,
    COLOR_08,
    COLOR_09,
    COLOR_10,
    COLOR_11,
    COLOR_12,
    COLOR_13,
    COLOR_14,
    COLOR_15,
    COLOR_16,
    COLOR_17,
    COLOR_18,
    COLOR_19,
    COLOR_20,
    COLOR_21,
    COLOR_22,
    COLOR_23,
    COLOR_24,
    COLOR_25,
    COLOR_26,
    COLOR_27,
    COLOR_28,
    COLOR_29,
    COLOR_30,
    COLOR_31
};

std::array<std::array<int, item::BLOCK_FACE_COUNT>, item::TOTAL_BLOCKS> item::blocks = {
    // w => (left, right, top, bottom, front, back) tiles
    0, 0, 0, 0, 0, 0, // 0 - empty
    16, 16, 32, 0, 16, 16, // 1 - grass
    1, 1, 1, 1, 1, 1, // 2 - sand
    2, 2, 2, 2, 2, 2, // 3 - stone
    3, 3, 3, 3, 3, 3, // 4 - brick
    20, 20, 36, 4, 20, 20, // 5 - wood
    5, 5, 5, 5, 5, 5, // 6 - cement
    6, 6, 6, 6, 6, 6, // 7 - dirt
    7, 7, 7, 7, 7, 7, // 8 - plank
    24, 24, 40, 8, 24, 24, // 9 - snow
    9, 9, 9, 9, 9, 9, // 10 - glass
    10, 10, 10, 10, 10, 10, // 11 - cobble
    11, 11, 11, 11, 11, 11, // 12 - light stone
    12, 12, 12, 12, 12, 12, // 13 - dark stone
    13, 13, 13, 13, 13, 13, // 14 - chest
    14, 14, 14, 14, 14, 14, // 15 - leaves
    15, 15, 15, 15, 15, 15, // 16 - cloud
    0, 0, 0, 0, 0, 0, // 17
    0, 0, 0, 0, 0, 0, // 18
    0, 0, 0, 0, 0, 0, // 19
    0, 0, 0, 0, 0, 0, // 20
    0, 0, 0, 0, 0, 0, // 21
    0, 0, 0, 0, 0, 0, // 22
    0, 0, 0, 0, 0, 0, // 23
    55, 55, 55, 55, 55, 55, // 24
    56, 56, 56, 56, 56, 56, // 25
    57, 57, 57, 57, 57, 57, // 26
    58, 58, 58, 58, 58, 58, // 27
    0, 0, 0, 0, 0, 0, // 28
    0, 0, 0, 0, 0, 0, // 29
    0, 0, 0, 0, 0, 0, // 30
    0, 0, 0, 0, 0, 0, // 31
    176, 176, 176, 176, 176, 176, // 32
    177, 177, 177, 177, 177, 177, // 33
    178, 178, 178, 178, 178, 178, // 34
    179, 179, 179, 179, 179, 179, // 35
    180, 180, 180, 180, 180, 180, // 36
    181, 181, 181, 181, 181, 181, // 37
    182, 182, 182, 182, 182, 182, // 38
    183, 183, 183, 183, 183, 183, // 39
    184, 184, 184, 184, 184, 184, // 40
    185, 185, 185, 185, 185, 185, // 41
    186, 186, 186, 186, 186, 186, // 42
    187, 187, 187, 187, 187, 187, // 43
    188, 188, 188, 188, 188, 188, // 44
    189, 189, 189, 189, 189, 189, // 45
    190, 190, 190, 190, 190, 190, // 46
    191, 191, 191, 191, 191, 191, // 47
    192, 192, 192, 192, 192, 192, // 48
    193, 193, 193, 193, 193, 193, // 49
    194, 194, 194, 194, 194, 194, // 50
    195, 195, 195, 195, 195, 195, // 51
    196, 196, 196, 196, 196, 196, // 52
    197, 197, 197, 197, 197, 197, // 53
    198, 198, 198, 198, 198, 198, // 54
    199, 199, 199, 199, 199, 199, // 55
    200, 200, 200, 200, 200, 200, // 56
    201, 201, 201, 201, 201, 201, // 57
    202, 202, 202, 202, 202, 202, // 58
    203, 203, 203, 203, 203, 203, // 59
    204, 204, 204, 204, 204, 204, // 60
    205, 205, 205, 205, 205, 205, // 61
    206, 206, 206, 206, 206, 206, // 62
    207, 207, 207, 207, 207, 207, // 63
};

std::array<int, item::TOTAL_PLANTS> item::plants = {
    // w => tile
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 16
    48, // 17 - tall grass
    49, // 18 - yellow flower
    50, // 19 - red flower
    51, // 20 - purple flower
    52, // 21 - sun flower
    53, // 22 - white flower
    54, // 23 - blue flower
    0,  // 24 - SDL_LOGO
    0,  // 25 - SFML_LOGO
    57, // 26 - cactus 1
    58, // 27 - cactus 2
};

bool item::is_plant(const int w) noexcept {
    switch (w) {
        case TALL_GRASS:
        case YELLOW_FLOWER:
        case RED_FLOWER:
        case PURPLE_FLOWER:
        case SUN_FLOWER:
        case WHITE_FLOWER:
        case BLUE_FLOWER:
        case CACTUS_1:
        case CACTUS_2:
            return true;
        default:
            return false;
    }
}

bool item::is_obstacle(int w) noexcept {
    w = SDL_abs(w);
    if (is_plant(w)) {
        return false;
    }
    switch (w) {
        case EMPTY:
        case CLOUD:
            return false;
        default:
            return true;
    }
}

bool item::is_transparent(int w) noexcept {
    if (w == EMPTY) {
        return true;
    }
    w = SDL_abs(w);
    if (is_plant(w)) {
        return true;
    }
    switch (w) {
        case EMPTY:
        case GLASS:
        case LEAVES:
            return true;
        default:
            return false;
    }
}

bool item::is_destructable(const int w) noexcept {
    switch (w) {
        case EMPTY:
        case CLOUD:
            return false;
        default:
            return true;
    }
}
