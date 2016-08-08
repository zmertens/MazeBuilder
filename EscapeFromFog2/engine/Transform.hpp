#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <memory>

#include <glm/glm.hpp>

class Transform
{
public:
    typedef std::unique_ptr<Transform> Ptr;
public:
    Transform();
    Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale);

    glm::mat4 getModel() const;
    glm::mat4 getModelCentered() const;
    glm::mat4 getModelView(const glm::mat4& view) const;
    glm::mat4 getMVP(const glm::mat4& view, const glm::mat4& projection) const;

    glm::vec3 getTranslation() const;
    void setTranslation(const glm::vec3& translation);
    glm::vec3 getRotation() const;
    void setRotation(const glm::vec3& rotation);
    glm::vec3 getScale() const;
    void setScale(const glm::vec3& scale);
private:
    glm::vec3 mTranslation;
    glm::vec3 mRotation;
    glm::vec3 mScale;
};

#endif // TRANSFORM_HPP
