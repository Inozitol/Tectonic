#ifndef TECTONIC_LODMANAGER_H
#define TECTONIC_LODMANAGER_H

#include <vector>

#include "utils/Utils.h"
#include "engine/camera/Camera.h"
#include "utils/Logger.h"
#include "defs/ConfigDefs.h"
#include "math/Transformation.h"

struct Terrain;

struct LODManager {
    void init(Terrain* terrain);

    struct patchLOD{
        uint8_t core = 0;
        uint8_t left = 0;
        uint8_t right = 0;
        uint8_t top = 0;
        uint8_t bottom = 0;
    };

    [[nodiscard]] const patchLOD& getPatchLOD(uint32_t patchX, uint32_t patchY) const;
    void calcLODRegions();
    void loadHeightsPerPatch(const std::vector<std::vector<float>>& map);

    void updateLODMapPass1(uint32_t patchX, uint32_t patchY);
    void updateLODMapPass1(const glm::vec3& camPos);
    void updateLODMapPass2();

    uint32_t distanceToLOD(float distance);


    Terrain* terrain;

    std::vector<std::vector<patchLOD>> m_map;
    std::vector<std::vector<float>> m_heights;
    std::vector<float> m_regions;

    Signal<> sig_LODChange;

    Slot<uint32_t, uint32_t> slt_activePatch{[this](const uint32_t patchX, const uint32_t patchY) {
        updateLODMapPass1(patchX, patchY);
        updateLODMapPass2();
        sig_LODChange.emit();
    }};
    Slot<const glm::vec3&> slt_cameraPosition{[this](const glm::vec3& pos){
        updateLODMapPass1(pos);
        updateLODMapPass2();
        sig_LODChange.emit();
    }};

};


#endif //TECTONIC_LODMANAGER_H
