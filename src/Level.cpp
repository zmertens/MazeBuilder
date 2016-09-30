#include "Level.hpp"

#include "engine/ResourceManager.hpp"
#include "engine/Camera.hpp"
#include "engine/SdlWindow.hpp"
#include "engine/Utils.hpp"

/**
 * @brief Level::Level
 * @param wallTex
 * @param floorTex
 * @param ceilTex
 * @param texAtlasRows
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Level::Level(
    const unsigned int wallTex, const unsigned int floorTex, const unsigned int ceilTex,
    float texAtlasRows,
    const Draw::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: mConfig(config)
, mTransform(position, rotation, scale)
, cTileScalar(20.0f, 20.0f, 20.0f)
, cSpriteHalfWidth((cTileScalar.x + cTileScalar.z) * 0.25f)
, mLevel(::LEVEL_ONE)
, mVertices()
, mIndices()
, mWallTexId(wallTex)
, mFloorTexId(floorTex)
, mCeilTexId(ceilTex)
, mTexAtlasRows(texAtlasRows)
{
    generateLevel();
}

/**
 * @brief Level::update
 * @param dt
 * @param timeSinceInit
 */
void Level::update(float dt, double timeSinceInit)
{

}

/**
 * @brief Level::draw
 * @param sdlManager
 * @param rm
 * @param camera
 * @param type = IMesh::Draw::TRIANGLES
 */
void Level::draw(const SdlWindow& sdlManager,
    ResourceManager& rm,
    const Camera& camera,
    const IMesh::Draw type) const
{
    auto& shader = rm.getShader(mConfig.shaderId);
    shader->bind();

    auto& tex = rm.getTexture(mConfig.textureId);
    tex->bind();

    auto mv = mTransform.getModelView(camera.getLookAt());
    auto persp = camera.getPerspective(sdlManager.getAspectRatio());
    shader->setUniform("uProjMatrix", persp);
    shader->setUniform("uModelViewMatrix", mv);

    auto& mat = rm.getMaterial(mConfig.materialId);
    auto& mesh = rm.getMesh(mConfig.meshId);

    shader->setUniform("uMaterial.ambient", mat->getAmbient());
    shader->setUniform("uMaterial.diffuse", mat->getDiffuse());
    shader->setUniform("uMaterial.specular", mat->getSpecular());
    shader->setUniform("uMaterial.shininess", mat->getShininess());

    mesh->draw(type);
} // draw

void Level::cleanUp()
{

}

/**
 * @brief Level::generateLevel
 */
void Level::generateLevel() noexcept
{
    using namespace Tile;

    for (std::size_t i = 0; i != mLevel.size(); ++i)
    {
        for (std::size_t j = 0; j != mLevel.at(0).size(); ++j)
        {
            if (mLevel[i][j].empty == true)
            {
                mEmptySpace.emplace_back(i, 0, j);
                continue;
            }
            else
            {
                addSpecial(mLevel[i][j].special, i, j);

                generateFloor(i, j);

                generateCeiling(i, j);

                if (mLevel[i][j - 1].empty == true)
                    generateWall(i, j, 'N');
                if (mLevel[i][j + 1].empty == true)
                    generateWall(i, j, 'S');
                if (mLevel[i - 1][j].empty == true)
                    generateWall(i, j, 'W');
                if (mLevel[i + 1][j].empty == true)
                    generateWall(i, j, 'E');
            }
        }
    }
} // generateLevel


std::vector<Vertex> Level::getVertices() const
{
    return mVertices;
}

std::vector<GLushort> Level::getIndices() const
{
    return mIndices;
}

/**
 * @brief Level::getExitPoints
 * @return
 */
std::vector<glm::vec3> Level::getExitPoints() const
{
    return mExitPoints;
}


/**
 * @brief Level::getEmptySpace
 * @return
 */
std::vector<glm::vec3> Level::getEmptySpace() const
{
    return mEmptySpace;
}

/**
 * @brief Level::getPlayerPosition
 * @return
 */
glm::vec3 Level::getPlayerPosition() const
{
    return mPlayerPosition;
}

/**
 * @brief Level::getTileScalar
 * @return
 */
glm::vec3 Level::getTileScalar() const
{
    return cTileScalar;
}

/**
 * @brief Level::getEnemyPositions
 * @return
 */
std::vector<glm::vec3> Level::getEnemyPositions() const
{
    return mEnemyPositions;
}

/**
 * @brief Level::getSpriteHalfWidth
 * @return
 */
float Level::getSpriteHalfWidth() const
{
    return cSpriteHalfWidth;
}

/**
 * @brief Level::getSpeedPowerUps
 * @return
 */
std::vector<glm::vec3> Level::getSpeedPowerUps() const
{
    return mSpeedPowerUps;
}

/**
 * @brief Level::getStrengthPowerUps
 * @return
 */
std::vector<glm::vec3> Level::getStrengthPowerUps() const
{
    return mStrengthPowerUps;
}

/**
 * @brief Level::getInvinciblePowerUps
 * @return
 */
std::vector<glm::vec3> Level::getInvinciblePowerUps() const
{
    return mInvinciblePowerUps;
}

/**
 * @brief Level::getTexCoordsFromOffset
 * @param texCoord
 * @param offset
 * @return
 */
glm::vec2 Level::getTexCoordsFromOffset(const glm::vec2& texCoord, const glm::vec2& offset) const
{
    return (texCoord / mTexAtlasRows) + offset;
}

/**
 * @brief Level::addSpecial
 * @param special
 * @param x
 * @param z
 */
void Level::addSpecial(Tile::Special special, std::size_t x, std::size_t z)
{
    using namespace Tile;
    switch (special)
    {
        case Special::PLAYER:
            mPlayerPosition = glm::vec3((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::ENEMY:
            mEnemyPositions.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::EXIT:
            mExitPoints.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::IMMUN_PW:
            mInvinciblePowerUps.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::STR_PW:
            mStrengthPowerUps.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::SPD_PW:
            mSpeedPowerUps.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
    } // switch
}

/**
 * @brief Level::generateFloor
 * @param i
 * @param j
 */
void Level::generateFloor(std::size_t i, std::size_t j)
{
    mIndices.push_back(mVertices.size() + 2);
    mIndices.push_back(mVertices.size() + 1);
    mIndices.push_back(mVertices.size() + 0);
    mIndices.push_back(mVertices.size() + 3);
    mIndices.push_back(mVertices.size() + 2);
    mIndices.push_back(mVertices.size() + 0);

    glm::vec2 low = getTexCoordsFromOffset(glm::vec2(0.0f, 0.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));
    glm::vec2 lowUhighV = getTexCoordsFromOffset(glm::vec2(0.0f, 1.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));
    glm::vec2 high = getTexCoordsFromOffset(glm::vec2(1.0f, 1.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));
    glm::vec2 highUlowV = getTexCoordsFromOffset(glm::vec2(1.0f, 0.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));

    mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(0, 1, 0)));
    mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, j * cTileScalar.z), lowUhighV, glm::vec3(0, 1, 0)));
    mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, (j + 1) * cTileScalar.z), high, glm::vec3(0, 1, 0)));
    mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(0, 1, 0)));
}

/**
 * @brief Level::generateCeiling
 * @param i
 * @param j
 */
void Level::generateCeiling(std::size_t i, std::size_t j)
{
    mIndices.push_back(mVertices.size() + 0);
    mIndices.push_back(mVertices.size() + 1);
    mIndices.push_back(mVertices.size() + 2);
    mIndices.push_back(mVertices.size() + 0);
    mIndices.push_back(mVertices.size() + 2);
    mIndices.push_back(mVertices.size() + 3);

    glm::vec2 low = getTexCoordsFromOffset(glm::vec2(0.0f, 0.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));
    glm::vec2 lowUhighV = getTexCoordsFromOffset(glm::vec2(0.0f, 1.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));
    glm::vec2 high = getTexCoordsFromOffset(glm::vec2(1.0f, 1.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));
    glm::vec2 highUlowV = getTexCoordsFromOffset(glm::vec2(1.0f, 0.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));

    mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), low, glm::vec3(0, 1, 0)));
    mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(0, 1, 0)));
    mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(0, 1, 0)));
    mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(0, 1, 0)));
}

/**
 * @brief Level::generateWall
 * @param i
 * @param j
 * @param dir
 */
void Level::generateWall(std::size_t i, std::size_t j, char dir)
{
    glm::vec2 low = getTexCoordsFromOffset(glm::vec2(0.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));
    glm::vec2 lowUhighV = getTexCoordsFromOffset(glm::vec2(0.0f, 1.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));
    glm::vec2 high = getTexCoordsFromOffset(glm::vec2(1.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));
    glm::vec2 highUlowV = getTexCoordsFromOffset(glm::vec2(1.0f, 0.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));

    switch (dir)
    {
        case 'N':
        {
            mIndices.push_back(mVertices.size() + 0);
            mIndices.push_back(mVertices.size() + 1);
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 0);
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 3);

            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(0, 0, 1)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, j * cTileScalar.z), highUlowV, glm::vec3(0, 0, 1)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), high, glm::vec3(0, 0, 1)));
            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(0, 0, 1)));
            break;
        }
        case 'S':
        {
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 1);
            mIndices.push_back(mVertices.size() + 0);
            mIndices.push_back(mVertices.size() + 3);
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 0);

            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, (j + 1) * cTileScalar.z), low, glm::vec3(0, 0, 1)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(0, 0, 1)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(0, 0, 1)));
            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), lowUhighV, glm::vec3(0, 0, 1)));
            break;
        }
        case 'W':
        {
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 1);
            mIndices.push_back(mVertices.size() + 0);
            mIndices.push_back(mVertices.size() + 3);
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 0);

            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(1, 0, 0)));
            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(1, 0, 0)));
            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(1, 0, 0)));
            mVertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(1, 0, 0)));
            break;
        }
        case 'E':
        {
            mIndices.push_back(mVertices.size() + 0);
            mIndices.push_back(mVertices.size() + 1);
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 0);
            mIndices.push_back(mVertices.size() + 2);
            mIndices.push_back(mVertices.size() + 3);

            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(1, 0, 0)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(1, 0, 0)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(1, 0, 0)));
            mVertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(1, 0, 0)));
            break;
        }
    } // switch
} // generate wall
