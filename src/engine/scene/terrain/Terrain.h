#pragma once

#include <random>
#include <limits>
#include <utility>
#include <defs/CompilerDefs.h>

#include "LODManager.h"
#include "PatchManager.h"
#include "engine/scene/RenderableBuffer.h"
#include "engine/scene/model/Model.h"
#include "utils/Utils.h"

#define MAX_TERRAIN_HEIGHT_TEXTURE 4

struct Terrain {
    using Vertex_t = VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::STATIC>;

    Terrain();
    ~Terrain();

    void clear();

    /**
     * @brief Generates a flat terrain with given dimensions.
     * @param dimX Amount of vertices in X dimension.
     * @param dimZ Amount of vertices in Z dimension.
     */
    void generateFlat(uint32_t dimX, uint32_t dimZ);

    /**
     * @brief Loads a heightmap from a file.
     * @param heightmapFile Heightmap file path.
     * @param textureFile Texture file path.
     */
    void loadHeightmap(const char *heightmapFile, const char *textureFile);

    /**
     * @brief Generates a terrain with midpoint algorithm.
     * @param size Terrain size dimension.
     * @param roughness Roughness factor.
     * @param textureFiles Vector of textures.
     */
    void generateMidpoint(uint32_t size, float roughness, const std::vector<std::string> &textureFiles);

    /**
     * @brief Sets a maximum amount of LOD levels per patch. Adjusts the patch size accordingly.
     * @param maxLOD Maximum amount of levels of details per one patch (results in patch size of 2^maxLOD + 1)
     */
    void setMaxLOD(uint32_t maxLOD);

    void initSignals();

    void connectCamera(Camera &camera);

    [[nodiscard]] std::pair<uint32_t, uint32_t> getCenterCoords() const;

    float hMapLCoord(uint32_t x, uint32_t y);
    float hMapLCoord(std::pair<uint32_t, uint32_t> coords);
    const glm::vec3 &pMapWCoord(int32_t x, int32_t y);
    float hMapWCoord(int32_t x, int32_t y);
    float hMapBaryWCoord(float x, float y);

    using blendingTexturesArray_t = std::array<std::pair<float, VktTypes::Resources::Image>, MAX_TERRAIN_HEIGHT_TEXTURE>;

    struct renderIterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = VktTypes::TerrainRenderObject;
        using pointer = VktTypes::TerrainRenderObject const *;
        using reference = VktTypes::TerrainRenderObject const &;

        std::function<value_type(uint32_t &, uint32_t &)> nextMesh;

        explicit renderIterator(std::function<value_type(uint32_t &, uint32_t &)> func) : nextMesh(std::move(func)), m_done(false) {
            m_mesh = nextMesh(m_patchX, m_patchY);
            if(m_patchX == 0 && m_patchY == 0) m_done = true;
        }

        explicit operator bool() const { return !m_done; }

        reference operator*() const { return m_mesh; }
        pointer operator->() const { return &m_mesh; }

        renderIterator &operator++() {
            m_mesh = nextMesh(m_patchX, m_patchY);
            if(m_patchX == 0 && m_patchY == 0) m_done = true;
            return *this;
        }

        renderIterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(renderIterator const &lhs, renderIterator const &rhs) { return (lhs.m_done && rhs.m_done); }

        friend bool operator!=(renderIterator const &lhs, renderIterator const &rhs) { return (!lhs.m_done || !rhs.m_done); }

        renderIterator(renderIterator &&) = default;
        renderIterator(renderIterator const &) = default;
        renderIterator &operator=(renderIterator &&) = default;
        renderIterator &operator=(renderIterator const &) = default;
        renderIterator() = delete;

    private:
        bool m_done;
        value_type m_mesh{};
        uint32_t m_patchX = 0;
        uint32_t m_patchY = 0;
    };

    [[nodiscard]] renderIterator renderIter() const;

    enum class Flags : std::uint8_t {
        SET_NEAREST_SIZE,
        CULL_PATCHES,
        MAX
    };

    Utils::Flags<Flags> flags;

    /** Height index to X,Y coordinate */
    [[nodiscard]] std::pair<uint32_t, uint32_t> hi2xy(uint32_t i) const { return {i % dimX, i / dimY}; }

    /** Height X,Y coordinate to index */
    [[nodiscard]] uint32_t hxy2i(uint32_t x, uint32_t y) const { return (y * dimX) + x; }

    /** Patch index to X,Y coordinate */
    [[nodiscard]] std::pair<uint32_t, uint32_t> pi2xy(uint32_t i) const { return {i % patchesX, i / patchesY}; }

    /** Patch X,Y coordinate to index */
    [[nodiscard]] uint32_t pxy2i(uint32_t x, uint32_t y) const { return (y * patchesX) + x; }

    /**
     * @brief Get heightmap height at coordinate.
     */
    always_inline float &hMapAt(uint32_t x, uint32_t y) { return vertices.at((y * dimX) + x).position.y; }

    /**
     * @brief Get heightmap height at index.
     */
    always_inline float &hMapAt(uint32_t i) { return vertices.at(i).position.y; }

    /**
     * @brief Get heightmap point at coordinate.
     */
    always_inline glm::vec3 &pMapAt(uint32_t x, uint32_t y) { return vertices.at((y * dimX) + x).position; }

    /**
     * @brief Get heightmap point at index.
     */
    always_inline glm::vec3 &pMapAt(uint32_t i) { return vertices.at(i).position; }

    void generateFlatPlane();
    void createPatchIndices();
    void createPatchIndicesLOD(uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom);
    void createFan(uint32_t x, uint32_t y, uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom);
    void createTriangle(uint32_t i0, uint32_t i1, uint32_t i2);
    void normalize();
    void calcMinMax();
    void calcNormals();

    void createMaterial();
    void createPatchesObjects();

    void diamondStep(uint32_t rectSize, float currHeight);
    void squareStep(uint32_t rectSize, float currHeight);

    bool isPatchInsideFrustum(uint32_t x, uint32_t y);

    void generateDebugLines();

    float worldScale = 1.0f;

    uint32_t dimX = 0;
    uint32_t dimY = 0;

    uint32_t patchesX = 0;
    uint32_t patchesY = 0;

    float minHeight = std::numeric_limits<float>::infinity();
    float maxHeight = -std::numeric_limits<float>::infinity();

    float minRange = 0.0f;
    float maxRange = 50.0f;

    std::vector<VktTypes::MaterialInstance> materials;

    DescriptorAllocatorDynamic descriptorPool;
    VktTypes::Resources::Buffer materialBuffer;

    uint32_t maxLOD = 0;
    uint32_t patchSize = 0;

    std::vector<VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::STATIC>> vertices;
    std::vector<uint32_t> indices;
    VktTypes::GPU::MeshBuffers meshBuffers;

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

    using patchID_t = uint32_t;

    struct PatchObject {
        uint32_t patchX = 0;
        uint32_t patchY = 0;
        LODManager::patchLOD pLOD;
        bool culled = false;
    };

    std::vector<LODInfo> lodInfo;
    LODManager lodManager;
    PatchManager patchManager;

    RenderableBuffer<PatchObject, VktTypes::TerrainRenderObject, patchID_t> patchObjects{
            [](const PatchObject &) { return 1; },
            [this](const PatchObject &obj, VktTypes::DrawContext<VktTypes::TerrainRenderObject> &ctx, const std::size_t offset) {
                const LODManager::patchLOD &pLOD = obj.pLOD;
                const uint8_t C = pLOD.core;
                const uint8_t L = pLOD.left;
                const uint8_t R = pLOD.right;
                const uint8_t T = pLOD.top;
                const uint8_t B = pLOD.bottom;

                const uint32_t baseIndex = lodInfo.at(C).info[L][R][T][B].start;

                const uint32_t x = obj.patchX * (patchSize - 1);
                const uint32_t y = obj.patchY * (patchSize - 1);
                const uint32_t baseVertex = y * dimX + x;
                if(ctx.opaqueRenderable.size() <= offset) ctx.opaqueRenderable.resize(offset+1);
                if(flags[Flags::CULL_PATCHES] && obj.culled) {
                    ctx.opaqueRenderable.at(offset) = VktTypes::TerrainRenderObject{.culled = true};
                }else{
                    ctx.opaqueRenderable.at(offset) = VktTypes::TerrainRenderObject{
                        .indexCount = lodInfo.at(C).info[L][R][T][B].count,
                        .firstIndex = baseIndex,
                        .vertexOffset = static_cast<int32_t>(baseVertex),
                        .indexBuffer = meshBuffers.indexBuffer.buffer,
                        .material = &materials.at(0),
                        .vertexBufferAddress = meshBuffers.vertexBufferAddress
                    };
                }
            },
    };

    VktTypes::PointMesh debugGridLines;

    /**
     * The patch iteration function should change the coordinate to the next patch to be processed.
     * By changing this function the engine can enable or disable patch culling.
     */
    using patchIterFunc_t = std::function<void(uint32_t &, uint32_t &)>;

    /**
     * Modifies the patch coordinates to point to the next patch in order.
     */
    patchIterFunc_t patchIterNormal = {
            [this](uint32_t &patchX, uint32_t &patchY) {
                if(++patchX == patchesX) {
                    ++patchY;
                    patchX = 0;
                }
            }
    };

    /**
     * Modifies the patch coordinates to point to the next patch in order while culling unseen patches.
     */
    patchIterFunc_t patchIterFrustumCulling = {
            [this](uint32_t &patchX, uint32_t &patchY) {
                do {
                    patchX++;
                    if(patchX == patchesX) {
                        patchY++;
                        patchX = 0;
                    }
                    if(patchY == patchesY) break;
                } while(!isPatchInsideFrustum(patchX, patchY));
            }
    };

    patchIterFunc_t patchIterFunc = patchIterNormal;

    /**
     * This function should return the next mesh to be rendered
     * It should calculate the correct LOD level and indices selection.
     */
    std::function<VktTypes::TerrainRenderObject(uint32_t &patchX, uint32_t &patchY)> meshFunc = {
            [this](uint32_t &patchX, uint32_t &patchY) {
                if(patchY >= patchesY) {
                    patchX = 0;
                    patchY = 0;
                    return VktTypes::TerrainRenderObject();
                }

                const LODManager::patchLOD &pLOD = lodManager.getPatchLOD(patchX, patchY);
                const uint32_t C = pLOD.core;
                const uint32_t L = pLOD.left;
                const uint32_t R = pLOD.right;
                const uint32_t T = pLOD.top;
                const uint32_t B = pLOD.bottom;

                const uint32_t baseIndex = lodInfo.at(C).info[L][R][T][B].start;

                const uint32_t x = patchX * (patchSize - 1);
                const uint32_t y = patchY * (patchSize - 1);
                const uint32_t baseVertex = y * dimX + x;

                const VktTypes::TerrainRenderObject renderObject{
                        .indexCount = lodInfo.at(C).info[L][R][T][B].count,
                        .firstIndex = baseIndex,
                        .vertexOffset = static_cast<int32_t>(baseVertex),
                        .indexBuffer = meshBuffers.indexBuffer.buffer,
                        .material = &materials.at(0),
                        .vertexBufferAddress = meshBuffers.vertexBufferAddress
                };

                patchIterFunc(patchX, patchY);

                return renderObject;
            }
    };

    Utils::FrustumCulling frustumCulling = Utils::FrustumCulling{.bias = -0.1};

    Slot<Flags, bool> slt_flagChange{
            [this](Flags flag, bool state) {
                switch(flag) {
                    case Flags::CULL_PATCHES:
                        if(state) { patchIterFunc = patchIterFrustumCulling; } else { patchIterFunc = patchIterNormal; }
                        break;
                    default:
                        break;
                }
            }
    };

    Slot<> slt_LODChange{
        [this]() {
            if(flags[Flags::CULL_PATCHES]) {
                for(uint32_t pX = 0; pX < patchesX; pX++) {
                    for(uint32_t pY = 0; pY < patchesY; pY++) {
                        uint32_t pI = pxy2i(pX,pY);
                        if(!isPatchInsideFrustum(pX, pY)) {
                            patchObjects.objects.data(pI).culled = true;
                            continue;
                        }
                        patchObjects.objects.data(pI).patchX = pX;
                        patchObjects.objects.data(pI).patchY = pY;
                        patchObjects.objects.data(pI).pLOD = lodManager.getPatchLOD(pX, pY);
                    }
                }
            }else{
                for(uint32_t pX = 0; pX < patchesX; pX++) {
                    for(uint32_t pY = 0; pY < patchesY; pY++) {
                        uint32_t pI = pxy2i(pX,pY);
                        patchObjects.objects.data(pI).patchX = pX;
                        patchObjects.objects.data(pI).patchY = pY;
                        patchObjects.objects.data(pI).pLOD = lodManager.getPatchLOD(pX, pY);
                    }
                }
            }
            for(uint32_t pX = 0; pX < patchesX; pX++) {
                for(uint32_t pY = 0; pY < patchesY; pY++) {
                    patchObjects.objectChanged(pxy2i(pX,pY));
                }
            }

        }
    };

    static inline std::random_device randDevice;
};