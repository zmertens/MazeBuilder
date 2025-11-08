#include "Font.hpp"

#include <dearimgui/imgui.h>

/// @brief Load font from memory (compressed TTF data)
/// @param compressedData Pointer to the compressed TTF data in memory
/// @param compressedSize Size of the compressed data in bytes
/// @param pixelSize Desired pixel size for the font
/// @return true if the font was loaded successfully, false otherwise
bool Font::loadFromMemoryCompressedTTF(const void* compressedData, std::size_t compressedSize, float pixelSize)
{
    mFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(compressedData, static_cast<int>(compressedSize),
                                                                 pixelSize);

    IM_ASSERT(mFont != nullptr);

    return true;
}

ImFont* Font::get() const
{
    return mFont;
}
