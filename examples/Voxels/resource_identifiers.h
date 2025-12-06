#ifndef RESOURCE_IDENTIFIERS_H
#define RESOURCE_IDENTIFIERS_H

#include <string_view>

enum class TextureIdentifier : unsigned int
{
    ATLAS = 0,
    CHARACTER = 1,
    BITMAP_FONT = 2,
    WINDOW_ICON = 3,
    SIGNS = 4,
    TOTAL = 5
};

enum class FontIdentifier : unsigned int
{
    COUSINE_REGULAR = 0,
    LIMELIGHT = 1,
    NUNITO_SANS = 2
};

class texture;
class font;

// Forward declaration and a few type definitions
template <typename Resource, typename Identifier>
class resource_manager;

typedef resource_manager<texture, TextureIdentifier> texture_manager;
typedef resource_manager<font, FontIdentifier> font_manager;

#endif // RESOURCE_IDENTIFIERS_H
