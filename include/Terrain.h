#ifndef TECTONIC_TERRAIN_H
#define TECTONIC_TERRAIN_H

#include "Model.h"

class Terrain : public Model {
public:
    Terrain() = default;
    ~Terrain() = default;

    /**
     * @brief Creates a flat terrain with given dimensions.
     * @param dimX Amount of vertices in X dimension.
     * @param dimZ Amount of vertices in Z dimension.
     */
    void createTerrain(uint32_t dimX, uint32_t dimZ, const char* textureFile);

private:
    uint32_t m_dimX;
    uint32_t m_dimZ;
};


#endif //TECTONIC_TERRAIN_H
