#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <memory>

#include <glm/glm.hpp>

class Material
{
public:
    typedef std::unique_ptr<Material> Ptr;
public:
    explicit Material();
    explicit Material(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
        float shininess);
    explicit Material(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
        float shininess, float reflectValue, float refractValue);

    glm::vec3 getAmbient() const;
    void setAmbient(const glm::vec3& ambient);

    glm::vec3 getDiffuse() const;
    void setDiffuse(const glm::vec3& diffuse);

    glm::vec3 getSpecular() const;
    void setSpecular(const glm::vec3& specular);

    float getShininess() const;
    void setShininess(float shininess);

    float getReflectivity() const;
    void setReflectivity(float reflectivity);

    float getRefractivity() const;
    void setRefractivity(float refractivity);
private:
    glm::vec3 mAmbient;
    glm::vec3 mDiffuse;
    glm::vec3 mSpecular;
    float mShininess;
    float mReflectivity;
    float mRefractivity;
};

#endif // MATERIAL_HPP
