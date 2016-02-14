#include "MaterialFactory.hpp"

/**
 * @brief MaterialFactory::ProduceMaterial
 * @param type
 * @return
 */
Material::Ptr MaterialFactory::ProduceMaterial(const MaterialFactory::Types& type)
{
    switch (type)
    {
        case Types::EMERALD:
        {
            Material::Ptr emerald (new Material(
                glm::vec3(0.0215f, 0.1745f, 0.0215f),
                glm::vec3(0.07568f, 0.61424f, 0.07568f),
                glm::vec3(0.633f, 0.727811f, 0.633f),
                76.8f)
            );
            return emerald;
        }
        case Types::JADE:
        {
            Material::Ptr jade (new Material(
                glm::vec3(0.135f, 0.2225f, 0.0215f),
                glm::vec3(0.54f, 0.89f, 0.63f),
                glm::vec3(0.316228f, 0.316228f, 0.316228f),
                12.8f)
            );
            return jade;
        }
        case Types::OBSIDIAN:
        {
            Material::Ptr obsidian (new Material(
                glm::vec3(0.05375f, 0.05f, 0.06625f),
                glm::vec3(0.18275f, 0.17f, 0.22525f),
                glm::vec3(0.332741f, 0.328634f, 0.346435f),
                38.4f)
            );
            return obsidian;
        }
        case Types::PEARL:
        {
            Material::Ptr pearl (new Material(
                glm::vec3(0.25f, 0.20725f, 0.20725f),
                glm::vec3(1.0f, 0.829f, 0.829f),
                glm::vec3(0.296648f, 0.296648f, 0.296648f),
                11.264f)
            );
            return pearl;
        }
        case Types::WHITE:
        {
            Material::Ptr white (new Material(
                glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), 100.0f)
            );
            return white;
        }
        case Types::CORAL_ORANGE:
        {
            Material::Ptr coral (new Material(
                glm::vec3(1.0f, 0.5f, 0.31f),
                glm::vec3(1.0f, 0.5f, 0.31f),
                glm::vec3(1.0f, 0.5f, 0.31f), 13.725490f)
            );
            return coral;
        }
    } // end switch
} // ProduceMaterial

