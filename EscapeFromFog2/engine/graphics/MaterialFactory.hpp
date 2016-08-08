#ifndef MATERIALFACTORY_HPP
#define MATERIALFACTORY_HPP

#include <memory>

#include "Material.hpp"

/**
 * @brief The MaterialFactory class
 */
class MaterialFactory
{
public:
    enum class Types {
        EMERALD,
        JADE,
        OBSIDIAN,
        PEARL,
        RUBY,
        TURQOISE,
        BRASS,
        BRONZE,
        CHROME,
        COPPER,
        GOLD,
        SILVER,
        BLACK_PLASTIC,
        CYAN_PLASTIC,
        GREEN_PLASTIC,
        RED_PLASTIC,
        WHITE_PLASTIC,
        YELLOW_PLASTIC,
        BLACK_RUBBER,
        CYAN_RUBBER,
        GREEN_RUBBER,
        RED_RUBBER,
        WHITE_RUBBER,
        YELLOW_RUBBER,
        WHITE,
        CORAL_ORANGE
    };
public:
    static Material::Ptr ProduceMaterial(const MaterialFactory::Types& type);
};

#endif // MATERIALFACTORY_HPP
