#ifndef FONT_HPP
#define FONT_HPP

#include <cwchar>

struct ImFont;

class Font
{
public:
    // Load font from memory (compressed data)
    bool loadFromMemoryCompressedTTF(const void* compressedData, std::size_t compressedSize, float pixelSize);

    ImFont* get() const;

private:
    ImFont* mFont;
};

#endif // FONT_HPP

