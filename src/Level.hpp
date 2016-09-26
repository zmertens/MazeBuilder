#ifndef Level_HPP
#define Level_HPP

#include <vector>

#include <glm/glm.hpp>

#include "engine/graphics/MeshFactory.hpp"
#include "engine/graphics/IDrawable.hpp"
#include "engine/Transform.hpp"
#include "engine/Vertex.hpp"
#include "engine/SdlWindow.hpp"

#include "ResourceConstants.hpp"

class ResourceManager;
class Camera;

namespace Tile
{
enum class Special {
    PLAYER,
    ENEMY,
    SPD_PW,
    STR_PW,
    IMMUN_PW,
    EXIT,
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

namespace 
{
    //namespace Tx = ResourceIds::Textures;
using namespace Tile;

const std::vector<std::vector<Data>> LEVEL_ONE = {{
    {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::PLAYER}, {}, {}, {}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::SPD_PW}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {false,  Special::ENEMY}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::NONE}, {false, Special::ENEMY}, {false,  Special::IMMUN_PW}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {false,Special::NONE}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {false,Special::ENEMY}, {false,Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,Special::STR_PW}, {}, {}, {}, {false,  Special::ENEMY}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::ENEMY}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::ENEMY}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::EXIT}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {}, {}},
    {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}}
}};
} // namespace

class Level final : IDrawable
{
public:
    explicit Level(
        const unsigned int wallTex, const unsigned int floorTex, const unsigned int ceilTex,
        const float texAtlasRows,
        const Draw::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(const SdlWindow& sdlManager,
        ResourceManager& rm,
        const Camera& camera,
        const IMesh::Draw type = IMesh::Draw::TRIANGLES) const override;
    virtual void cleanUp() override;

    void generateLevel(std::vector<Vertex>& vertices, std::vector<GLushort>& indices);

    std::vector<glm::vec3> getExitPoints() const;

    std::vector<glm::vec3> getEmptySpace() const;

    glm::vec3 getPlayerPosition() const;

    glm::vec3 getTileScalar() const;

    std::vector<glm::vec3> getEnemyPositions() const;

    float getSpriteHalfWidth() const;

    std::vector<glm::vec3> getSpeedPowerUps() const;

    std::vector<glm::vec3> getStrengthPowerUps() const;

    std::vector<glm::vec3> getInvinciblePowerUps() const;

private:
    const glm::vec3 cTileScalar;
    const float cSpriteHalfWidth;
    std::vector<std::vector<Tile::Data>> mLevel;
    Draw::Config mConfig;
    Transform mTransform;
    unsigned int mWallTexId;
    unsigned int mFloorTexId;
    unsigned int mCeilTexId;
    float mTexAtlasRows;
    std::vector<glm::vec3> mEmptySpace;
    glm::vec3 mPlayerPosition;
    std::vector<glm::vec3> mExitPoints;
    std::vector<glm::vec3> mEnemyPositions;
    std::vector<glm::vec3> mSpeedPowerUps;
    std::vector<glm::vec3> mStrengthPowerUps;
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
