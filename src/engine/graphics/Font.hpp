#ifndef FONT_HPP
#define FONT_HPP

#include <map>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "../SdlWindow.hpp"

/**
*   To create the texture atlas:
*   https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02
*   It is assumed that the font atlas takes up only 1 row in the texture atlas
*/
class Font
{
    public:
        typedef std::unique_ptr<Font> Ptr;
    public:
        struct Character {
            glm::vec2 size;
            glm::vec2 bearing;
            GLfloat advance;
            GLfloat uOffset; // x offset in texture coordinates
        };
    public:
        explicit Font(const SdlWindow& sdl, const std::string& fileId, const long& fontHeight);
        virtual ~Font();
        void cleanUp();
        const std::string& getFileId() const;
        const long getFontHeight() const;
        const std::map<GLchar, Character>& getCharacters() const;
        const Font::Character& getCharacter(GLchar charToGet) const;
        GLuint getAtlasHeight() const;
        GLuint getAtlasWidth() const;
        void bindTexture() const;
    private:
        const std::string& mFileId;
        const long mFontHeight;
        std::map<GLchar, Font::Character> mCharacters;
        unsigned char* mBufferStr;
        long mBufferSize;
        GLuint mAtlasWidth;
        GLuint mAtlasHeight;
        GLuint mTex_Handle;
    private:
        Font(const Font& other);
        Font& operator=(const Font& other);
        void genTexHandle();
        void initGlyphs();
};

#endif // FONT_HPP
