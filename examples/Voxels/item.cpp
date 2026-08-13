#include "item.h"

#include <cmath>
#include <SDL3/SDL_stdinc.h>

std::array<int, item::TOTAL_ITEMS> item::ITEMS = {
    // items the user can build
    static_cast<int>(item::BlockType::GRASS),
    static_cast<int>(item::BlockType::SAND),
    static_cast<int>(item::BlockType::STONE),
    static_cast<int>(item::BlockType::BRICK),
    static_cast<int>(item::BlockType::WOOD),
    static_cast<int>(item::BlockType::CEMENT),
    static_cast<int>(item::BlockType::DIRT),
    static_cast<int>(item::BlockType::PLANK),
    static_cast<int>(item::BlockType::SNOW),
    static_cast<int>(item::BlockType::GLASS),
    static_cast<int>(item::BlockType::COBBLE),
    static_cast<int>(item::BlockType::LIGHT_STONE),
    static_cast<int>(item::BlockType::DARK_STONE),
    static_cast<int>(item::BlockType::CHEST),
    static_cast<int>(item::BlockType::LEAVES),
    static_cast<int>(item::BlockType::TALL_GRASS),
    static_cast<int>(item::BlockType::YELLOW_FLOWER),
    static_cast<int>(item::BlockType::RED_FLOWER),
    static_cast<int>(item::BlockType::PURPLE_FLOWER),
    static_cast<int>(item::BlockType::SUN_FLOWER),
    static_cast<int>(item::BlockType::WHITE_FLOWER),
    static_cast<int>(item::BlockType::BLUE_FLOWER),
    static_cast<int>(item::BlockType::SDL_LOGO),
    static_cast<int>(item::BlockType::SFML_LOGO),
    static_cast<int>(item::BlockType::CACTUS_1),
    static_cast<int>(item::BlockType::CACTUS_2),
    static_cast<int>(item::BlockType::COLOR_00),
    static_cast<int>(item::BlockType::COLOR_01),
    static_cast<int>(item::BlockType::COLOR_02),
    static_cast<int>(item::BlockType::COLOR_03),
    static_cast<int>(item::BlockType::COLOR_04),
    static_cast<int>(item::BlockType::COLOR_05),
    static_cast<int>(item::BlockType::COLOR_06),
    static_cast<int>(item::BlockType::COLOR_07),
    static_cast<int>(item::BlockType::COLOR_08),
    static_cast<int>(item::BlockType::COLOR_09),
    static_cast<int>(item::BlockType::COLOR_10),
    static_cast<int>(item::BlockType::COLOR_11),
    static_cast<int>(item::BlockType::COLOR_12),
    static_cast<int>(item::BlockType::COLOR_13),
    static_cast<int>(item::BlockType::COLOR_14),
    static_cast<int>(item::BlockType::COLOR_15),
    static_cast<int>(item::BlockType::COLOR_16),
    static_cast<int>(item::BlockType::COLOR_17),
    static_cast<int>(item::BlockType::COLOR_18),
    static_cast<int>(item::BlockType::COLOR_19),
    static_cast<int>(item::BlockType::COLOR_20),
    static_cast<int>(item::BlockType::COLOR_21),
    static_cast<int>(item::BlockType::COLOR_22),
    static_cast<int>(item::BlockType::COLOR_23),
    static_cast<int>(item::BlockType::COLOR_24),
    static_cast<int>(item::BlockType::COLOR_25),
    static_cast<int>(item::BlockType::COLOR_26),
    static_cast<int>(item::BlockType::COLOR_27),
    static_cast<int>(item::BlockType::COLOR_28),
    static_cast<int>(item::BlockType::COLOR_29),
    static_cast<int>(item::BlockType::COLOR_30),
    static_cast<int>(item::BlockType::COLOR_31)
};

std::array<std::array<int, item::BLOCK_FACE_COUNT>, item::TOTAL_BLOCKS> item::BLOCKS = {
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

std::array<int, item::TOTAL_PLANTS> item::PLANTS = {
    // w => tile
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 16
    48, // 17 - tall grass
    49, // 18 - yellow flower
    50, // 19 - red flower
    51, // 20 - purple flower
    52, // 21 - sun flower
    53, // 22 - white flower
    54, // 23 - blue flower
    0, // 24 - SDL_LOGO
    0, // 25 - SFML_LOGO
    57, // 26 - cactus 1
    58, // 27 - cactus 2
};

bool item::is_plant(const int w) noexcept
{
    switch (w)
    {
    case static_cast<int>(item::BlockType::TALL_GRASS):
    case static_cast<int>(item::BlockType::YELLOW_FLOWER):
    case static_cast<int>(item::BlockType::RED_FLOWER):
    case static_cast<int>(item::BlockType::PURPLE_FLOWER):
    case static_cast<int>(item::BlockType::SUN_FLOWER):
    case static_cast<int>(item::BlockType::WHITE_FLOWER):
    case static_cast<int>(item::BlockType::BLUE_FLOWER):
    case static_cast<int>(item::BlockType::CACTUS_1):
    case static_cast<int>(item::BlockType::CACTUS_2):
        return true;
    default:
        return false;
    }
}

bool item::is_obstacle(int w) noexcept
{
    w = SDL_abs(w);
    if (is_plant(w))
    {
        return false;
    }
    switch (w)
    {
    case static_cast<int>(item::BlockType::EMPTY):
    case static_cast<int>(item::BlockType::CLOUD):
        return false;
    default:
        return true;
    }
}

bool item::is_transparent(int w) noexcept
{
    if (w == static_cast<int>(item::BlockType::EMPTY))
    {
        return true;
    }
    w = SDL_abs(w);
    if (is_plant(w))
    {
        return true;
    }
    switch (w)
    {
    case static_cast<int>(item::BlockType::EMPTY):
    case static_cast<int>(item::BlockType::GLASS):
    case static_cast<int>(item::BlockType::LEAVES):
        return true;
    default:
        return false;
    }
}

bool item::is_destructable(const int w) noexcept
{
    switch (w)
    {
    case static_cast<int>(item::BlockType::EMPTY):
    case static_cast<int>(item::BlockType::CLOUD):
        return false;
    default:
        return true;
    }
}

item::BlockType item::get_block_type(int w) noexcept
{
    return static_cast<item::BlockType>(w);
}
