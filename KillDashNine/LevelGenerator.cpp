#include "LevelGenerator.hpp"

#include "engine/ResourceManager.hpp"
#include "engine/Camera.hpp"
#include "engine/SdlManager.hpp"
#include "engine/Utils.hpp"

/**
 * @brief LevelGenerator::LevelGenerator
 * @param level
 * @param wallTex
 * @param floorTex
 * @param ceilTex
 * @param texAtlasRows
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
LevelGenerator::LevelGenerator(const std::vector<std::vector<Tile::Data>>& level,
    const unsigned int wallTex, const unsigned int floorTex, const unsigned int ceilTex,
    float texAtlasRows,
    const Entity::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: Entity(config, position, rotation, scale)
, cTileScalar(20.0f, 20.0f, 20.0f)
, cSpriteHalfWidth((cTileScalar.x + cTileScalar.z) * 0.25f)
, mLevel(level)
, mWallTexId(wallTex)
, mFloorTexId(floorTex)
, mCeilTexId(ceilTex)
, mTexAtlasRows(texAtlasRows)
{

}

/**
 * @brief LevelGenerator::update
 * @param dt
 * @param timeSinceInit
 */
void LevelGenerator::update(float dt, double timeSinceInit)
{

}

/**
 * @brief LevelGenerator::draw
 * @param sdlManager
 * @param rm
 * @param camera
 * @param type = IMesh::Draw::TRIANGLES
 */
void LevelGenerator::draw(const SdlManager& sdlManager,
    ResourceManager& rm,
    const Camera& camera,
    const IMesh::Draw type) const
{
    // every config in the list has the same shader and texture
    auto& frontConfig = mConfig.front();
    auto& shader = rm.getShader(frontConfig.shaderId);
    if (!rm.isInCache(frontConfig.shaderId, CachePos::Shader))
    {
        rm.putInCache(frontConfig.shaderId, CachePos::Shader);
        shader->bind();
    }

    auto& tex = rm.getTexture(frontConfig.textureId);
    if (!rm.isInCache(frontConfig.textureId, CachePos::Texture))
    {
        rm.putInCache(frontConfig.textureId, CachePos::Texture);
        tex->bind();
    }

    auto mv = mTransform.getModelView(camera.getLookAt());
    auto persp = camera.getPerspective(sdlManager.getAspectRatio());
    shader->setUniform("uProjMatrix", persp);
    shader->setUniform("uModelViewMatrix", mv);

    for (auto& config : mConfig)
    {
        auto& mat = rm.getMaterial(config.materialId);
        auto& mesh = rm.getMesh(config.meshId);

        shader->setUniform("uMaterial.ambient", mat->getAmbient());
        shader->setUniform("uMaterial.diffuse", mat->getDiffuse());
        shader->setUniform("uMaterial.specular", mat->getSpecular());
        shader->setUniform("uMaterial.shininess", mat->getShininess());

        //shader->setUniform("uTexOffset0", config.texOffset0);

        mesh->draw(type);
    }
} // draw

/**
 * @brief LevelGenerator::generateLevel
 * @param vertices
 * @param indices
 */
void LevelGenerator::generateLevel(std::vector<Vertex>& vertices,
    std::vector<GLushort>& indices)
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

                generateFloor(vertices, indices, i, j);

                generateCeiling(vertices, indices, i, j);

                if (mLevel[i][j - 1].empty == true)
                    generateWall(vertices, indices, i, j, 'N');
                if (mLevel[i][j + 1].empty == true)
                    generateWall(vertices, indices, i, j, 'S');
                if (mLevel[i - 1][j].empty == true)
                    generateWall(vertices, indices, i, j, 'W');
                if (mLevel[i + 1][j].empty == true)
                    generateWall(vertices, indices, i, j, 'E');
            }
        }
    }
} // generateLevel

/**
 * @brief LevelGenerator::getExitPoints
 * @return
 */
std::vector<glm::vec3> LevelGenerator::getExitPoints() const
{
    return mExitPoints;
}


/**
 * @brief LevelGenerator::getEmptySpace
 * @return
 */
std::vector<glm::vec3> LevelGenerator::getEmptySpace() const
{
    return mEmptySpace;
}

/**
 * @brief LevelGenerator::getPlayerPosition
 * @return
 */
glm::vec3 LevelGenerator::getPlayerPosition() const
{
    return mPlayerPosition;
}

/**
 * @brief LevelGenerator::getTileScalar
 * @return
 */
glm::vec3 LevelGenerator::getTileScalar() const
{
    return cTileScalar;
}

/**
 * @brief LevelGenerator::getEnemyPositions
 * @return
 */
std::vector<glm::vec3> LevelGenerator::getEnemyPositions() const
{
    return mEnemyPositions;
}

/**
 * @brief LevelGenerator::getSpriteHalfWidth
 * @return
 */
float LevelGenerator::getSpriteHalfWidth() const
{
    return cSpriteHalfWidth;
}

/**
 * @brief LevelGenerator::getSpeedPowerUps
 * @return
 */
std::vector<glm::vec3> LevelGenerator::getSpeedPowerUps() const
{
    return mSpeedPowerUps;
}

/**
 * @brief LevelGenerator::getRechargePowerUps
 * @return
 */
std::vector<glm::vec3> LevelGenerator::getRechargePowerUps() const
{
    return mRechargePowerUps;
}

/**
 * @brief LevelGenerator::getInvinciblePowerUps
 * @return
 */
std::vector<glm::vec3> LevelGenerator::getInvinciblePowerUps() const
{
    return mInvinciblePowerUps;
}

/**
 * @brief LevelGenerator::getTexCoordsFromOffset
 * @param texCoord
 * @param offset
 * @return
 */
glm::vec2 LevelGenerator::getTexCoordsFromOffset(const glm::vec2& texCoord, const glm::vec2& offset) const
{
    return (texCoord / mTexAtlasRows) + offset;
}

/**
 * @brief LevelGenerator::addSpecial
 * @param special
 * @param x
 * @param z
 */
void LevelGenerator::addSpecial(Tile::Special special, std::size_t x, std::size_t z)
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
        case Special::INVINC_PW:
            mInvinciblePowerUps.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::RCHRG_PW:
            mRechargePowerUps.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
        case Special::SPD_PW:
            mSpeedPowerUps.emplace_back((x + 0.5f) * cTileScalar.x, cTileScalar.y * 0.5f, (z + 0.5f) * cTileScalar.z);
            break;
    } // switch
}

/**
 * @brief LevelGenerator::generateFloor
 * @param vertices
 * @param indices
 * @param i
 * @param j
 */
void LevelGenerator::generateFloor(std::vector<Vertex>& vertices, std::vector<GLushort>& indices, std::size_t i, std::size_t j)
{
    indices.push_back(vertices.size() + 2);
    indices.push_back(vertices.size() + 1);
    indices.push_back(vertices.size() + 0);
    indices.push_back(vertices.size() + 3);
    indices.push_back(vertices.size() + 2);
    indices.push_back(vertices.size() + 0);

    glm::vec2 low = getTexCoordsFromOffset(glm::vec2(0.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));
    glm::vec2 lowUhighV = getTexCoordsFromOffset(glm::vec2(0.0f, 1.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));
    glm::vec2 high = getTexCoordsFromOffset(glm::vec2(1.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));
    glm::vec2 highUlowV = getTexCoordsFromOffset(glm::vec2(1.0f, 0.0f), Utils::getTexAtlasOffset(mFloorTexId, mTexAtlasRows));

    vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(0, 1, 0)));
    vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, j * cTileScalar.z), lowUhighV, glm::vec3(0, 1, 0)));
    vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, (j + 1) * cTileScalar.z), high, glm::vec3(0, 1, 0)));
    vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(0, 1, 0)));
}

/**
 * @brief LevelGenerator::generateCeiling
 * @param vertices
 * @param indices
 * @param i
 * @param j
 */
void LevelGenerator::generateCeiling(std::vector<Vertex>& vertices, std::vector<GLushort>& indices, std::size_t i, std::size_t j)
{
    indices.push_back(vertices.size() + 0);
    indices.push_back(vertices.size() + 1);
    indices.push_back(vertices.size() + 2);
    indices.push_back(vertices.size() + 0);
    indices.push_back(vertices.size() + 2);
    indices.push_back(vertices.size() + 3);

    glm::vec2 low = getTexCoordsFromOffset(glm::vec2(0.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));
    glm::vec2 lowUhighV = getTexCoordsFromOffset(glm::vec2(0.0f, 1.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));
    glm::vec2 high = getTexCoordsFromOffset(glm::vec2(1.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));
    glm::vec2 highUlowV = getTexCoordsFromOffset(glm::vec2(1.0f, 0.0f), Utils::getTexAtlasOffset(mCeilTexId, mTexAtlasRows));

    vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), low, glm::vec3(0, 1, 0)));
    vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(0, 1, 0)));
    vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(0, 1, 0)));
    vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(0, 1, 0)));
}

/**
 * @brief LevelGenerator::generateWall
 * @param vertices
 * @param indices
 * @param i
 * @param j
 * @param dir
 */
void LevelGenerator::generateWall(std::vector<Vertex>& vertices, std::vector<GLushort>& indices, std::size_t i, std::size_t j, char dir)
{
    glm::vec2 low = getTexCoordsFromOffset(glm::vec2(0.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));
    glm::vec2 lowUhighV = getTexCoordsFromOffset(glm::vec2(0.0f, 1.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));
    glm::vec2 high = getTexCoordsFromOffset(glm::vec2(1.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));
    glm::vec2 highUlowV = getTexCoordsFromOffset(glm::vec2(1.0f, 0.0f), Utils::getTexAtlasOffset(mWallTexId, mTexAtlasRows));

    switch (dir)
    {
        case 'N':
        {
            indices.push_back(vertices.size() + 0);
            indices.push_back(vertices.size() + 1);
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 0);
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 3);

            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(0, 0, 1)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, j * cTileScalar.z), highUlowV, glm::vec3(0, 0, 1)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), high, glm::vec3(0, 0, 1)));
            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(0, 0, 1)));
            break;
        }
        case 'S':
        {
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 1);
            indices.push_back(vertices.size() + 0);
            indices.push_back(vertices.size() + 3);
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 0);

            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, (j + 1) * cTileScalar.z), low, glm::vec3(0, 0, 1)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(0, 0, 1)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(0, 0, 1)));
            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), lowUhighV, glm::vec3(0, 0, 1)));
            break;
        }
        case 'W':
        {
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 1);
            indices.push_back(vertices.size() + 0);
            indices.push_back(vertices.size() + 3);
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 0);

            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(1, 0, 0)));
            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(1, 0, 0)));
            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(1, 0, 0)));
            vertices.push_back(Vertex(glm::vec3(i * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(1, 0, 0)));
            break;
        }
        case 'E':
        {
            indices.push_back(vertices.size() + 0);
            indices.push_back(vertices.size() + 1);
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 0);
            indices.push_back(vertices.size() + 2);
            indices.push_back(vertices.size() + 3);

            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, j * cTileScalar.z), low, glm::vec3(1, 0, 0)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, 0, (j + 1) * cTileScalar.z), highUlowV, glm::vec3(1, 0, 0)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, (j + 1) * cTileScalar.z), high, glm::vec3(1, 0, 0)));
            vertices.push_back(Vertex(glm::vec3((i + 1) * cTileScalar.x, cTileScalar.y, j * cTileScalar.z), lowUhighV, glm::vec3(1, 0, 0)));
            break;
        }
    } // switch
} // generate wall
