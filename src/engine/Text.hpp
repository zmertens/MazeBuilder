#ifndef TEXT_HPP
#define TEXT_HPP

#include <glm/glm.hpp>
#include <string>
#include <memory>

#include "SdlWindow.hpp"
#include "BoundingBox.hpp"

class Text
{
    public:
        typedef std::unique_ptr<Text> Ptr;
    public:
        explicit Text();
        explicit Text(const std::string& str, const BoundingBox& box, 
            const std::string& fontId, glm::vec3 color = glm::vec3(1.0f), glm::vec3 scale = glm::vec3(1.0f));

        const std::string& getText() const;
        const BoundingBox& getBox() const;
        const std::string& getFontId() const;
        glm::vec3 getColor() const;
        glm::vec3 getScale() const;

        void setText(const std::string& text);
        void setBox(const BoundingBox& box);
        void setColor(const glm::vec3& color);
        void setScale(const glm::vec3& scale);
    private:
        std::string mText;
        BoundingBox mBox;
        std::string mFontId;
        glm::vec3 mColor;
        glm::vec3 mScale;
};

#endif // TEXT_HPP
