#ifndef ITEM_H
#define ITEM_H

#include <array>

class item
{
public:
    enum class BlockType : int
    {
        EMPTY = 0,
        GRASS = 1,
        SAND = 2,
        STONE = 3,
        BRICK = 4,
        WOOD = 5,
        CEMENT = 6,
        DIRT = 7,
        PLANK = 8,
        SNOW = 9,
        GLASS = 10,
        COBBLE = 11,
        LIGHT_STONE = 12,
        DARK_STONE = 13,
        CHEST = 14,
        LEAVES = 15,
        CLOUD = 16,
        TALL_GRASS = 17,
        YELLOW_FLOWER = 18,
        RED_FLOWER = 19,
        PURPLE_FLOWER = 20,
        SUN_FLOWER = 21,
        WHITE_FLOWER = 22,
        BLUE_FLOWER = 23,
        SDL_LOGO = 24,
        SFML_LOGO = 25,
        CACTUS_1 = 26,
        CACTUS_2 = 27,
        COLOR_00 = 32,
        COLOR_01 = 33,
        COLOR_02 = 34,
        COLOR_03 = 35,
        COLOR_04 = 36,
        COLOR_05 = 37,
        COLOR_06 = 38,
        COLOR_07 = 39,
        COLOR_08 = 40,
        COLOR_09 = 41,
        COLOR_10 = 42,
        COLOR_11 = 43,
        COLOR_12 = 44,
        COLOR_13 = 45,
        COLOR_14 = 46,
        COLOR_15 = 47,
        COLOR_16 = 48,
        COLOR_17 = 49,
        COLOR_18 = 50,
        COLOR_19 = 51,
        COLOR_20 = 52,
        COLOR_21 = 53,
        COLOR_22 = 54,
        COLOR_23 = 55,
        COLOR_24 = 56,
        COLOR_25 = 57,
        COLOR_26 = 58,
        COLOR_27 = 59,
        COLOR_28 = 60,
        COLOR_29 = 61,
        COLOR_30 = 62,
        COLOR_31 = 63,
    };

    static constexpr auto TOTAL_BLOCKS = 256;
    static constexpr auto BLOCK_FACE_COUNT = 6;
    static constexpr auto TOTAL_ITEMS = 64;
    static constexpr auto TOTAL_PLANTS = 256;

    static bool is_plant(int w) noexcept;
    static bool is_obstacle(int w) noexcept;
    static bool is_transparent(int w) noexcept;
    static bool is_destructable(int w) noexcept;

    static BlockType get_block_type(int w) noexcept;

    static std::array<std::array<int, BLOCK_FACE_COUNT>, TOTAL_BLOCKS> BLOCKS;
    static std::array<int, TOTAL_ITEMS> ITEMS;
    static std::array<int, TOTAL_PLANTS> PLANTS;
};

#endif // ITEM_H
