#include "Font.hpp"

#include <algorithm> // for max

#include <ft2build.h>
#include FT_FREETYPE_H

Font::Font(const SdlWindow& sdl, const std::string& fileId, const long& fontHeight)
: mFileId(fileId)
, mFontHeight(fontHeight)
, mBufferSize(0l)
, mAtlasWidth(0u)
, mAtlasHeight(0u)
{
    genTexHandle();
    mBufferStr = sdl.buildBufferFromFile(mFileId, mBufferSize);
    initGlyphs();
}

Font::~Font()
{

}

void Font::cleanUp()
{
    free(mBufferStr);
    mBufferStr = nullptr;

    glDeleteTextures(1, &mTex_Handle);

    mCharacters.clear(); 
}

void Font::genTexHandle()
{
    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &mTex_Handle);
    glBindTexture(GL_TEXTURE_2D, mTex_Handle);
}

/**
*   Ensure that the texture handle is bound before initializing glyphs
*/
void Font::initGlyphs()
{
    const GLubyte StartingChar = 32;
    const GLubyte EndingChar = 122;

    // FreeType
    FT_Library ftLibrary;
    FT_Face ftFace;

    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ftLibrary))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font library: %s\n", SDL_GetError());
    }

    // Load font as face
    if (FT_New_Memory_Face(ftLibrary, mBufferStr, mBufferSize, 0, &ftFace))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font face: %s\n", SDL_GetError());
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(ftFace, 0, mFontHeight);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load 128 characters of ASCII set, skip first 32
    for (GLubyte asciiChar = StartingChar; asciiChar != EndingChar; ++asciiChar)
    {
        // Load character glyph
        if (FT_Load_Char(ftFace, asciiChar, FT_LOAD_RENDER))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Loading ASCII char %c failed\n", asciiChar);
            continue;
        }

        mAtlasWidth += ftFace->glyph->bitmap.width;
        mAtlasHeight = std::max(mAtlasHeight, ftFace->glyph->bitmap.rows);
    }

#if defined(APP_DEBUG)
    SDL_Log("font atlas width = %d, font atlas height = %d\n", mAtlasWidth, mAtlasHeight);
#endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mAtlasWidth, mAtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    GLuint xOffsetInAtlas = 0;

    // Load 128 characters of ASCII set, skip first 32
    for (GLubyte asciiChar = StartingChar; asciiChar != EndingChar; ++asciiChar)
    {
        // Load character glyph
        if (FT_Load_Char(ftFace, asciiChar, FT_LOAD_RENDER))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Loading ASCII char %c failed\n", asciiChar);
            continue;
        }

        // Now store character for later use
        Font::Character character = {
            glm::vec2(static_cast<GLfloat>(ftFace->glyph->bitmap.width), static_cast<GLfloat>(ftFace->glyph->bitmap.rows)),
            glm::vec2(static_cast<GLfloat>(ftFace->glyph->bitmap_left), static_cast<GLfloat>(ftFace->glyph->bitmap_top)),
            static_cast<GLfloat>(ftFace->glyph->advance.x >> 6), // the glyph spacing is 1 / 64 pixels
            static_cast<GLfloat>(xOffsetInAtlas) / static_cast<GLfloat>(mAtlasWidth)
        };

        mCharacters.insert(std::pair<GLchar, Font::Character>(asciiChar, character));

        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffsetInAtlas, 0, ftFace->glyph->bitmap.width, ftFace->glyph->bitmap.rows,
            GL_RED, GL_UNSIGNED_BYTE, ftFace->glyph->bitmap.buffer);

        xOffsetInAtlas += ftFace->glyph->bitmap.width;
    }

#if defined(APP_DEBUG)
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    // Destroy FreeType once we're finished
    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);
}

const std::string& Font::getFileId() const
{
    return mFileId;
}

const long Font::getFontHeight() const
{
    return mFontHeight;
}

const std::map<GLchar, Font::Character>& Font::getCharacters() const
{
    return mCharacters;
}

const Font::Character& Font::getCharacter(GLchar charToGet) const
{
    return mCharacters.at(charToGet);
}

GLuint Font::getAtlasHeight() const
{
    return mAtlasHeight;
}

GLuint Font::getAtlasWidth() const
{
    return mAtlasWidth;
}

void Font::bindTexture() const
{
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTex_Handle);
}
