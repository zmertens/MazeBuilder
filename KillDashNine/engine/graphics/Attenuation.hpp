// Example attenuation values:
// http://learnopengl.com/#!Lighting/Light-casters

#ifndef ATTENUATION_HPP
#define ATTENUATION_HPP

/**
 * @brief The Attenuation class
 */
class Attenuation
{
public:
    Attenuation()
    : constant(0.0f)
    , linear(0.0f)
    , quadratic(0.0f)
    {

    }

    Attenuation(float cnst, float lnr, float qdrtc)
    : constant(cnst)
    , linear(lnr)
    , quadratic(qdrtc)
    {

    }

public:
    float constant;
    float linear;
    float quadratic;
};

#endif // ATTENUATION_HPP
