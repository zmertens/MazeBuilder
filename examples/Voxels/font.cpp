#include "font.h"

#include <dearimgui/imgui.h>

/// @brief Load font from memory (compressed TTF data)
/// @param compressedData Pointer to the compressed TTF data in memory
/// @param compressedSize Size of the compressed data in bytes
/// @param pixelSize Desired pixel size for the font
/// @return true if the font was loaded successfully, false otherwise
bool font::loadFromMemoryCompressedTTF(const void* compressedData, const std::size_t compressedSize, const float pixelSize)
{
    m_font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(compressedData, static_cast<int>(compressedSize),
                                                                 pixelSize);

    IM_ASSERT(m_font != nullptr);

    return true;
}

ImFont* font::get() const
{
    return m_font;
}
