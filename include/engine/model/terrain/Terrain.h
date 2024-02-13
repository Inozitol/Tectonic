#ifndef TECTONIC_TERRAIN_H
#define TECTONIC_TERRAIN_H

#include "extern/stb/stb_image.h"
#include <random>
#include <limits>
#include <utility>

#include "engine/model/Model.h"
#include "Transformation.h"
#include "Logger.h"
#include "LODManager.h"

class Terrain : public Model {
    friend class EngineCore;
public:
    Terrain();
    ~Terrain() = default;

    void initMeta();

    /**
     * @brief Generates a flat terrain with given dimensions.
     * @param dimX Amount of vertices in X dimension.
     * @param dimZ Amount of vertices in Z dimension.
     * @param textureFile Texture file path.
     * @param normalFile Normal file path.
     */
    void generateFlat(uint32_t dimX, uint32_t dimZ, const char* textureFile, const char* normalFile = nullptr);

    /**
     * @brief Loads a heightmap from a file.
     * @param heightmapFile Heightmap file path.
     * @param textureFile Texture file path.
     */
    void loadHeightmap(const char* heightmapFile, const char* textureFile);

    /**
     * @brief Generates a terrain with midpoint algorithm.
     * @param size Terrain size dimension.
     * @param roughness Roughness factor.
     * @param textureFiles Vector of textures.
     */
    void generateMidpoint(uint32_t size, float roughness, const std::vector<std::string>& textureFiles);

    void setMaxRange(float maxRange);
    void setMinRange(float minRange);

    /**
     * @brief Sets a maximum amount of LOD levels per patch. Adjusts the patch size accordingly.
     * @param maxLOD Maximum amount of levels of details per one patch (results in patch size of 2^maxLOD + 1)
     */
    void setMaxLOD(uint32_t maxLOD);
    void setCamera(Camera& camera);
    void setScale(float scale);
    float getScale();

    [[nodiscard]] std::pair<float, float> getMinMaxHeight();
    [[nodiscard]] std::pair<uint32_t, uint32_t> getCenterCoords() const;
    float hMapLCoord(uint32_t x, uint32_t y);
    float hMapLCoord(std::pair<uint32_t, uint32_t> coords);

    const glm::vec3& pMapWCoord(int32_t x, int32_t y);
    float hMapWCoord(int32_t x, int32_t y);

    float hMapBaryWCoord(float x, float y);

    void clear() override;

    void bindBlendingTextures();

    using blendingTexturesArray_t = std::array<std::pair<float, std::shared_ptr<Texture>>, MAX_TERRAIN_HEIGHT_TEXTURE>;

    class meshIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = MeshInfo;
        using pointer           = MeshInfo const*;
        using reference         = MeshInfo const&;

        std::function<value_type(uint32_t&, uint32_t&)> nextMesh;

        explicit meshIterator(std::function<value_type(uint32_t&, uint32_t&)> func) : m_done(false), nextMesh(std::move(func)){
            m_mesh = nextMesh(m_patchX, m_patchY);
            if(m_patchX == 0 && m_patchY == 0)
                m_done = true;
        }

        explicit operator bool() const { return !m_done; }

        reference operator*() const { return m_mesh; }
        pointer operator->() const { return &m_mesh; }

        meshIterator& operator++(){
            m_mesh = nextMesh(m_patchX, m_patchY);
            if(m_patchX == 0 && m_patchY == 0)
                m_done = true;
            return *this;
        }

        meshIterator operator++(int){
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(meshIterator const& lhs, meshIterator const& rhs){
            return (lhs.m_done && rhs.m_done);
        }

        friend bool operator!=(meshIterator const& lhs, meshIterator const& rhs){
            return (!lhs.m_done || !rhs.m_done);
        }

        meshIterator(meshIterator&&) = default;
        meshIterator(meshIterator const&) = default;
        meshIterator& operator=(meshIterator &&) = default;
        meshIterator& operator=(meshIterator const&) = default;
        meshIterator() = delete;

    private:
        bool m_done;
        value_type m_mesh{};
        uint32_t m_patchX = 0;
        uint32_t m_patchY = 0;
    };

    meshIterator meshIter();

    enum class Flags : std::uint8_t{
        SET_NEAREST_SIZE,
        CULL_PATCHES,
        SIZE
    };

    Utils::Flags<Flags> flags;

private:
    void generateFlatPlane();
    [[nodiscard]] inline std::pair<uint32_t,uint32_t> i2xy(uint32_t i) const { return {i % m_dimX, i / m_dimY}; }
    [[nodiscard]] inline uint32_t xy2i(uint32_t x, uint32_t y) const { return (y*m_dimX)+x; }

    float& hMapAt(uint32_t x, uint32_t y);
    float& hMapAt(uint32_t i);

    glm::vec3& pMapAt(uint32_t x, uint32_t y);
    glm::vec3& pMapAt(uint32_t i);

    void createPatchIndices();
    void createPatchIndicesLOD(uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom);
    void createFan(uint32_t x, uint32_t y, uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom);
    void createTriangle(uint32_t i0, uint32_t i1, uint32_t i2);
    void normalize();
    void calcMinMax();
    void calcNormals();

    void addBlendTexture(float height, const std::shared_ptr<Texture>& texture);

    void diamondStep(uint32_t rectSize, float currHeight);
    void squareStep(uint32_t rectSize, float currHeight);

    bool isPatchInsideFrustum(uint32_t x, uint32_t y);

    float m_worldScale = 1.0f;

    uint32_t m_dimX = 0;
    uint32_t m_dimY = 0;

    uint32_t m_patchesX = 0;
    uint32_t m_patchesY = 0;

    float m_minHeight = std::numeric_limits<float>::infinity();
    float m_maxHeight = -std::numeric_limits<float>::infinity();

    float m_minRange = 0.0f;
    float m_maxRange = 50.0f;

    blendingTexturesArray_t m_blendingTextures;
    decltype(MAX_TERRAIN_HEIGHT_TEXTURE) m_blendingTexturesCount = 0;

    uint32_t m_maxLOD = 0;
    uint32_t m_patchSize = 0;

    constexpr static uint8_t LEFT = 2;
    constexpr static uint8_t RIGHT = 2;
    constexpr static uint8_t TOP = 2;
    constexpr static uint8_t BOTTOM = 2;

    struct singleLODInfo {
        uint32_t start = 0;
        uint32_t count = 0;
    };
    struct LODInfo {
        singleLODInfo info[LEFT][RIGHT][TOP][BOTTOM];
    };
    std::vector<LODInfo> m_lodInfo;
    LODManager m_lodManager;

    std::function<MeshInfo(uint32_t&, uint32_t&)> m_meshFunc = {[this](uint32_t& patchX, uint32_t& patchY){

        if (patchY >= m_patchesY) {
            patchX = 0;
            patchY = 0;
            return MeshInfo();
        }

        const LODManager::patchLOD& pLOD = m_lodManager.getPatchLOD(patchX, patchY);
        uint32_t C = pLOD.core;
        uint32_t L = pLOD.left;
        uint32_t R = pLOD.right;
        uint32_t T = pLOD.top;
        uint32_t B = pLOD.bottom;

        uint32_t baseIndex = m_lodInfo.at(C).info[L][R][T][B].start;

        uint32_t x = patchX * (m_patchSize-1);
        uint32_t y = patchY * (m_patchSize-1);
        uint32_t baseVertex = y * m_dimX + x;

        MeshInfo meshInfo;
        meshInfo.indicesCount = m_lodInfo.at(C).info[L][R][T][B].count;
        meshInfo.verticesOffset = baseVertex;
        meshInfo.indicesOffset = baseIndex;
        meshInfo.matIndex = 0;

        patchX++;
        if (patchX == m_patchesX) {
            patchY++;
            patchX = 0;
        }

        return meshInfo;
    }};

    Utils::FrustumCulling m_frustumCulling = Utils::FrustumCulling(0.1);

    Slot<Flags, bool> slt_flagChange {[this](Flags flag, bool state){
        switch(flag){
            case Flags::CULL_PATCHES:
                if(state){
                    m_meshFunc = {[this](uint32_t& patchX, uint32_t& patchY){
                        if (patchY >= m_patchesY) {
                            patchX = 0;
                            patchY = 0;
                            return MeshInfo();
                        }
                        const LODManager::patchLOD& pLOD = m_lodManager.getPatchLOD(patchX, patchY);
                        uint32_t C = pLOD.core;
                        uint32_t L = pLOD.left;
                        uint32_t R = pLOD.right;
                        uint32_t T = pLOD.top;
                        uint32_t B = pLOD.bottom;
                        MeshInfo meshInfo;
                        meshInfo.indicesCount = m_lodInfo.at(C).info[L][R][T][B].count;
                        meshInfo.verticesOffset = (patchY * (m_patchSize-1)) * m_dimX + (patchX * (m_patchSize-1));
                        meshInfo.indicesOffset = m_lodInfo.at(C).info[L][R][T][B].start;
                        meshInfo.matIndex = 0;
                        do {
                            patchX++;
                            if (patchX == m_patchesX) {
                                patchY++;
                                patchX = 0;
                            }
                            if (patchY == m_patchesY) break;

                        }while(!isPatchInsideFrustum(patchX, patchY));
                        return meshInfo;
                    }};
                }else{
                    m_meshFunc = {[this](uint32_t& patchX, uint32_t& patchY){
                        if (patchY >= m_patchesY) {
                            patchX = 0;
                            patchY = 0;
                            return MeshInfo();
                        }
                        const LODManager::patchLOD& pLOD = m_lodManager.getPatchLOD(patchX, patchY);
                        uint32_t C = pLOD.core;
                        uint32_t L = pLOD.left;
                        uint32_t R = pLOD.right;
                        uint32_t T = pLOD.top;
                        uint32_t B = pLOD.bottom;
                        MeshInfo meshInfo;
                        meshInfo.indicesCount = m_lodInfo.at(C).info[L][R][T][B].count;
                        meshInfo.verticesOffset = (patchY * (m_patchSize-1)) * m_dimX + (patchX * (m_patchSize-1));
                        meshInfo.indicesOffset = m_lodInfo.at(C).info[L][R][T][B].start;
                        meshInfo.matIndex = 0;
                        patchX++;
                        if (patchX == m_patchesX) {
                            patchY++;
                            patchX = 0;
                        }
                        return meshInfo;
                    }};
                }
                break;
            default: break;
        }
    }};

    std::random_device m_randDevice;
    static Logger m_logger;
};

#endif //TECTONIC_TERRAIN_H
