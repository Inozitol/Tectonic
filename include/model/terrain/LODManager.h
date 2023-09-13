#ifndef TECTONIC_LODMANAGER_H
#define TECTONIC_LODMANAGER_H

#include <vector>

#include "utils.h"
#include "../../camera/Camera.h"
#include "Logger.h"
#include "defs/ConfigDefs.h"
#include "Transformation.h"
#include "meta/meta.h"


class LODManager {
public:

    void init(uint32_t maxLOD, uint32_t patchesX, uint32_t patchesY, float scale);

    struct patchLOD{
        uint32_t core = 0;
        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t top = 0;
        uint32_t bottom = 0;
    };

    const patchLOD& getPatchLOD(uint32_t patchX, uint32_t patchY) const;
    void calcLODRegions();
    void loadHeightsPerPatch(const std::vector<std::vector<float>>& map);

    Slot<glm::vec3> slt_cameraPosition{[this](const glm::vec3& pos){
        updateLODMapPass1(pos);
        updateLODMapPass2(pos);
    }};

private:

    void updateLODMapPass1(const glm::vec3& camPos);
    void updateLODMapPass2(const glm::vec3& camPos);

    uint32_t distanceToLOD(float distance);

    uint32_t m_maxLOD = 0;
    uint32_t m_patchSize = 0;
    uint32_t m_patchesX = 0;
    uint32_t m_patchesY = 0;
    uint32_t m_dimX = 0;
    uint32_t m_dimY = 0;

    std::vector<std::vector<patchLOD>> m_map;
    std::vector<std::vector<float>> m_heights;
    std::vector<float> m_regions;

    float m_worldScale;

    static Logger m_logger;

};


#endif //TECTONIC_LODMANAGER_H
