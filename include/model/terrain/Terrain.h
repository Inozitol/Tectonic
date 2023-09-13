#ifndef TECTONIC_TERRAIN_H
#define TECTONIC_TERRAIN_H

#include "extern/stb_image.h"
#include <random>
#include <limits>
#include <utility>

#include "model/Model.h"
#include "Transformation.h"
#include "Logger.h"
#include "LODManager.h"

class Terrain : public Model {
    friend class Renderer;
public:
    Terrain() = default;
    ~Terrain() = default;

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

    std::pair<float, float> getMinMaxHeight();
    std::pair<uint32_t, uint32_t> getCenterCoords() const;
    float getHeight(uint32_t x, uint32_t y);
    float getHeight(std::pair<uint32_t, uint32_t> coords);

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

private:
    void generateFlatPlane();
    inline std::pair<uint32_t,uint32_t> i2xy(uint32_t i) const {return {i % m_dimX, i / m_dimY};}
    inline uint32_t xy2i(uint32_t x, uint32_t y) const {return (y*m_dimX)+x;}

    float& hMapAt(uint32_t x, uint32_t y);
    float& hMapAt(uint32_t i);

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

    float m_worldScale;

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

    std::random_device m_randDevice;
    static Logger m_logger;
};


#endif //TECTONIC_TERRAIN_H
