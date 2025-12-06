#ifndef FONT_H
#define FONT_H

#include <cwchar>

struct ImFont;

class font
{
public:
    // Load font from memory (compressed data)
    bool loadFromMemoryCompressedTTF(const void* compressedData, std::size_t compressedSize, float pixelSize);

    [[nodiscard]] ImFont* get() const;

private:
    ImFont* m_font = nullptr;
};

#endif // FONT_H

