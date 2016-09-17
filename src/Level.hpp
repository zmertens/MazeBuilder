#ifndef Level_HPP
#define Level_HPP

#include <vector>

#include <glm/glm.hpp>

#include "engine/graphics/MeshFactory.hpp"
#include "engine/graphics/Entity.hpp"
#include "engine/Vertex.hpp"

#include "ResourceConstants.hpp"

class ResourceManager;
class Camera;
class SdlWindow;

namespace Tile
{
enum class Special : Uint32 {
    PLAYER,
    ENEMY,
    DOOR,
    SPD_PW,
    RCHRG_PW,
    INVINC_PW,
    EXIT,
    PARTICLE, // @TODO - be more specific (smoke, fire... )
    POINT_LIGHT,
    SPOT_LIGHT,
    DIR_LIGHT,
    NONE
};

typedef struct Data {
    bool empty;
    Special special;

    Data(bool isEmpty = true,
        Special isSpecial = Special::NONE)
    : empty(isEmpty)
    , special(isSpecial)
    {

    }

} Data;

} // namespace Tile

namespace StartLevel
{

//namespace Tx = ResourceIds::Textures;
using namespace Tile;

const std::vector<std::vector<Data>> TEST_LEVEL = {{
    {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::PLAYER}, {}, {}, {}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::SPD_PW}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {false,  Special::ENEMY}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::NONE}, {false, Special::ENEMY}, {false,  Special::INVINC_PW}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {false,Special::NONE}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {false,Special::ENEMY}, {false,Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,Special::RCHRG_PW}, {}, {}, {}, {false,  Special::ENEMY}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::ENEMY}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::ENEMY}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::EXIT}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {}, {}},
    {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}}
}};
} // levels


class Level final : Entity
{
public:
    explicit Level(const std::vector<std::vector<Tile::Data>>& level,
        const unsigned int wallTex, const unsigned int floorTex, const unsigned int ceilTex,
        const float texAtlasRows,
        const Entity::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    virtual void update(float dt, double timeSinceInit);
    virtual void draw(const SdlWindow& sdlManager,
        ResourceManager& rm,
        const Camera& camera,
        const IMesh::Draw type = IMesh::Draw::TRIANGLES) const;

    void generateLevel(std::vector<Vertex>& vertices, std::vector<GLushort>& indices);

    std::vector<glm::vec3> getExitPoints() const;

    std::vector<glm::vec3> getEmptySpace() const;

    glm::vec3 getPlayerPosition() const;

    glm::vec3 getTileScalar() const;

    std::vector<glm::vec3> getEnemyPositions() const;

    float getSpriteHalfWidth() const;

    std::vector<glm::vec3> getSpeedPowerUps() const;

    std::vector<glm::vec3> getRechargePowerUps() const;

    std::vector<glm::vec3> getInvinciblePowerUps() const;

private:
    const glm::vec3 cTileScalar;
    const float cSpriteHalfWidth;
    std::vector<std::vector<Tile::Data>> mLevel;
    unsigned int mWallTexId;
    unsigned int mFloorTexId;
    unsigned int mCeilTexId;
    float mTexAtlasRows;
    std::vector<glm::vec3> mEmptySpace;
    glm::vec3 mPlayerPosition;
    std::vector<glm::vec3> mExitPoints;
    std::vector<glm::vec3> mEnemyPositions;
    std::vector<glm::vec3> mSpeedPowerUps;
    std::vector<glm::vec3> mRechargePowerUps;
    std::vector<glm::vec3> mInvinciblePowerUps;
    // doors
    // power ups
    // particles
    // lights
private:
    glm::vec2 getTexCoordsFromOffset(const glm::vec2& texCoord, const glm::vec2& offset) const;
    void addSpecial(Tile::Special special, std::size_t x, std::size_t z);
    void generateFloor(std::vector<Vertex>& vertices, std::vector<GLushort>& indices, std::size_t i, std::size_t j);
    void generateCeiling(std::vector<Vertex>& vertices, std::vector<GLushort>& indices, std::size_t i, std::size_t j);
    void generateWall(std::vector<Vertex>& vertices, std::vector<GLushort>& indices, std::size_t i, std::size_t j, char dir);
};

#endif // Level_HPP
