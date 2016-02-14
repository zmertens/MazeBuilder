#include "Material.hpp"

/**
 * @brief Material::Material
 */
Material::Material()
: mAmbient(glm::vec3(0))
, mDiffuse(glm::vec3(0))
, mSpecular(glm::vec3(0))
, mShininess(0)
, mReflectivity(0)
, mRefractivity(0)
{

}

/**
 * @brief Material::Material
 * @param ambient
 * @param diffuse
 * @param specular
 * @param shininess
 */
Material::Material(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
    float shininess)
: mAmbient(ambient)
, mDiffuse(diffuse)
, mSpecular(specular)
, mShininess(shininess)
, mReflectivity(0)
, mRefractivity(0)
{

}

/**
 * @brief Material::Material
 * @param ambient
 * @param diffuse
 * @param specular
 * @param shininess
 * @param reflectValue
 * @param refractValue
 */
Material::Material(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
    float shininess, float reflectValue, float refractValue)
: mAmbient(ambient)
, mDiffuse(diffuse)
, mSpecular(specular)
, mShininess(shininess)
, mReflectivity(reflectValue)
, mRefractivity(refractValue)
{

}

/**
 * @brief Material::getAmbient
 * @return
 */
glm::vec3 Material::getAmbient() const
{
    return mAmbient;
}

/**
 * @brief Material::setAmbient
 * @param ambient
 */
void Material::setAmbient(const glm::vec3& ambient)
{
    mAmbient = ambient;
}

/**
 * @brief Material::getDiffuse
 * @return
 */
glm::vec3 Material::getDiffuse() const
{
    return mDiffuse;
}

/**
 * @brief Material::setDiffuse
 * @param diffuse
 */
void Material::setDiffuse(const glm::vec3& diffuse)
{
    mDiffuse = diffuse;
}

/**
 * @brief Material::getSpecular
 * @return
 */
glm::vec3 Material::getSpecular() const
{
    return mSpecular;
}

/**
 * @brief Material::setSpecular
 * @param specular
 */
void Material::setSpecular(const glm::vec3& specular)
{
    mSpecular = specular;
}

/**
 * @brief Material::getShininess
 * @return
 */
float Material::getShininess() const
{
    return mShininess;
}

/**
 * @brief Material::setShininess
 * @param shininess
 */
void Material::setShininess(float shininess)
{
    mShininess = shininess;
}

/**
 * @brief Material::getReflectivity
 * @return
 */
float Material::getReflectivity() const
{
    return mReflectivity;
}

/**
 * @brief Material::setReflectivity
 * @param reflectivity
 */
void Material::setReflectivity(float reflectivity)
{
    mReflectivity = reflectivity;
}

/**
 * @brief Material::getRefractivity
 * @return
 */
float Material::getRefractivity() const
{
    return mRefractivity;
}

/**
 * @brief Material::setRefractivity
 * @param refractivity
 */
void Material::setRefractivity(float refractivity)
{
    mRefractivity = refractivity;
}
