#ifndef FONT_HPP
#define FONT_HPP

#include <cwchar>

struct ImFont;

class Font
{
public:
    // Load font from memory (compressed data)
    bool loadFromMemoryCompressedTTF(const void* compressedData, std::size_t compressedSize, float pixelSize);

    [[nodiscard]] ImFont* get() const;

private:
    ImFont* mFont = nullptr;
};

#endif // FONT_HPP

