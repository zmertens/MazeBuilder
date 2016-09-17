#include "RenderText.hpp"

#include "ResourceManager.hpp"
#include "Text.hpp"

/**
 * @brief RenderText::RenderText
 */
RenderText::RenderText()
: mVaoHandle(0)
{
    genBuffers();
    initMesh();
}

/**
 * @brief RenderText::update
 * @param dt
 * @param timeSinceInit
 */
void RenderText::update(float dt, double timeSinceInit)
{

}

/**
 * @brief RenderText::draw
 * @param type = IMesh::Draw::TRIANGLES
 * @param count = 4
 */
void RenderText::draw(IMesh::Draw type, const unsigned int count) const
{
    glBindVertexArray(mVaoHandle);
    glDrawArrays(getGlType(type), 0, count);
#if defined(APP_DEBUG)
    glBindVertexArray(0);
#endif // defined
}

/**
 *
 */
void RenderText::renderText(const ResourceManager& rm, const Text& text) const
{
    std::string str = text.getText();
    GLfloat xPosition = text.getBox().minCoord.x;
    GLfloat yPosition = text.getBox().minCoord.y;
    glm::vec3 scale = text.getScale();

    const std::map<GLchar, Font::Character>& charMap = rm.getFont(text.getFontId())->getCharacters();

    glBindVertexArray(mVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mVboHandle);

    // Iterate through all characters in the render text
    for (std::string::const_iterator character = str.begin(); character != str.end(); ++character)
    {
        const Font::Character& charFromFont = charMap.at(*character);

        GLfloat xpos = xPosition + charFromFont.bearing.x * scale.x;
        GLfloat ypos = yPosition + (charMap.at('H').bearing.y - charFromFont.bearing.y) * scale.y;

        GLfloat width = charFromFont.size.x * scale.x;
        GLfloat height = charFromFont.size.y * scale.y;

        GLfloat uWidth =  static_cast<GLfloat>(charFromFont.size.x) / static_cast<GLfloat>(rm.getFont(text.getFontId())->getAtlasWidth());
        GLfloat vHeight = static_cast<GLfloat>(charFromFont.size.y) / static_cast<GLfloat>(rm.getFont(text.getFontId())->getAtlasHeight());

        // Update VBO data for each character
        GLfloat vertices[6][4] = {
            { xpos,         ypos + height,   charFromFont.uOffset           , vHeight },
            { xpos + width, ypos,            charFromFont.uOffset + uWidth  , 0.0f },
            { xpos,         ypos,            charFromFont.uOffset           , 0.0f },

            { xpos,         ypos + height,   charFromFont.uOffset           , vHeight },
            { xpos + width, ypos + height,   charFromFont.uOffset + uWidth  , vHeight },
            { xpos + width, ypos,            charFromFont.uOffset + uWidth  , 0.0f }
        };

        // Update content of mVboHandle memory
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        xPosition += (static_cast<GLfloat>(charFromFont.advance)) * scale.x;
    } // for (...)

#if defined(APP_DEBUG)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

/**
 * @brief RenderText::cleanUp
 */
void RenderText::cleanUp()
{
    glDeleteVertexArrays(1, &mVaoHandle);
    glDeleteBuffers(1, &mVboHandle);
}

/**
 * @brief RenderText::genBuffers
 */
void RenderText::genBuffers()
{
    glGenVertexArrays(1, &mVaoHandle);
    glGenBuffers(1, &mVboHandle);
}

/**
 * @brief RenderText::initMesh
 */
void RenderText::initMesh()
{
    glBindVertexArray(mVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mVboHandle);
    // vertex data = six points with 4 values per point
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

#if defined(APP_DEBUG)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

/**
 * @brief RenderText::getGlType
 * @param type
 * @return
 */
GLenum RenderText::getGlType(IMesh::Draw type) const
{
    switch (type)
    {
        case IMesh::Draw::TRIANGLES: return GL_TRIANGLES;
        case IMesh::Draw::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case IMesh::Draw::LINES: return GL_LINES;
        case IMesh::Draw::POINTS: return GL_POINTS;
    }
}

