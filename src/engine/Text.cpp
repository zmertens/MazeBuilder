#include "Text.hpp"

Text::Text()
: mText(std::string(""))
, mBox(glm::vec3(0.0f), glm::vec3(0.0f))
, mFontId(std::string(""))
, mColor(glm::vec3(0.0f))
, mScale(glm::vec3(0.0f))
{

}

/**
*   @param color = glm::vec3(1.0f)
*   @param scale = glm::vec3(1.0f)
*/
Text::Text(const std::string& str, const BoundingBox& box, const std::string& fontId, glm::vec3 color, glm::vec3 scale)
: mText(str)
, mBox(box)
, mFontId(fontId)
, mColor(color)
, mScale(scale)
{

}

const std::string& Text::getText() const
{
    return mText;
}

const BoundingBox& Text::getBox() const
{
    return mBox;
}

const std::string& Text::getFontId() const
{
    return mFontId;
}

glm::vec3 Text::getColor() const
{
    return mColor;
}

glm::vec3 Text::getScale() const
{
    return mScale;
}

void Text::setText(const std::string& text)
{
    mText = text;
}

void Text::setBox(const BoundingBox& box)
{
    mBox = box;
}

void Text::setColor(const glm::vec3& color)
{
    mColor = color;
}

void Text::setScale(const glm::vec3& scale)
{
    mScale = scale;
}
