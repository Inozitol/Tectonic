#include "LODManager.h"
#include "Terrain.h"

void LODManager::init(Terrain* t) {
    assert(t);
    terrain = t;
    m_map.resize(t->patchesY);
    for(uint32_t y = 0; y < t->patchesY; y++){
        m_map.at(y).resize(t->patchesX);
    }

    m_regions.resize(t->maxLOD+1);
    calcLODRegions();
}

void LODManager::calcLODRegions() {
    uint32_t sum = 0;

    for(uint32_t i = 0; i <= terrain->maxLOD; i++){
        sum += i + 1;
    }

    float X = CAMERA_PPROJ_FAR / static_cast<float>(sum);

    float temp = 0;

    for(uint32_t i = 0; i <= terrain->maxLOD; i++){
        auto currRange = X * static_cast<float>(i+1);
        m_regions.at(i) = temp + currRange;
        temp += currRange;
        LOG(LOG_DEBUG, "Regions with LOD " << i << " set to range " << currRange);
    }
}

const LODManager::patchLOD &LODManager::getPatchLOD(uint32_t patchX, uint32_t patchY) const {
    return m_map.at(patchY).at(patchX);
}

void LODManager::updateLODMapPass1(uint32_t patchX, uint32_t patchY) {
    for(uint32_t lodMapY = 0; lodMapY < terrain->patchesY; lodMapY++){
        for(uint32_t lodMapX = 0; lodMapX < terrain->patchesX; lodMapX++) {
            patchLOD& pPatchLOD = m_map.at(lodMapY).at(lodMapX);
            for(uint8_t LODLevel = 0; LODLevel < terrain->maxLOD; LODLevel++) {
                if(Utils::unsignedDst(lodMapX,patchX) < LODLevel + 2 && Utils::unsignedDst(lodMapY,patchY) < LODLevel + 2) {
                    pPatchLOD.core = LODLevel;
                    goto next_patch;
                }
            }
            pPatchLOD.core = terrain->maxLOD;
            next_patch:
        }
    }

}

void LODManager::updateLODMapPass1(const glm::vec3 &camPos) {
    int32_t centerStep = static_cast<int32_t>(terrain->patchSize) / 2;
    for(uint32_t lodMapY = 0; lodMapY < terrain->patchesY; lodMapY++){
        for(uint32_t lodMapX = 0; lodMapX < terrain->patchesX; lodMapX++) {
            //m_map.at(lodMapY).at(lodMapX).core = 0;
            //continue;
            uint32_t x = lodMapX * (terrain->patchSize - 1) + centerStep;
            uint32_t y = lodMapY * (terrain->patchSize - 1) + centerStep;

            glm::vec3 patchCenter = glm::vec3((static_cast<float>(x) - static_cast<float>(terrain->dimX)/2) * terrain->worldScale,
                                              m_heights.at(lodMapY).at(lodMapX),
                                              (static_cast<float>(y) - static_cast<float>(terrain->dimY)/2) * terrain->worldScale);

            float distanceToCamera = glm::distance(patchCenter, camPos);

            uint32_t coreLOD = distanceToLOD(distanceToCamera);

            patchLOD* pPatchLOD = &m_map.at(lodMapY).at(lodMapX);
            pPatchLOD->core = coreLOD;
        }
    }
}

void LODManager::updateLODMapPass2() {

    for(uint32_t lodMapY = 0; lodMapY < terrain->patchesY; lodMapY++) {
        for (uint32_t lodMapX = 0; lodMapX < terrain->patchesX; lodMapX++) {
            uint32_t coreLOD = m_map.at(lodMapY).at(lodMapX).core;

            uint32_t indexLeft = lodMapX;
            uint32_t indexRight = lodMapX;
            uint32_t indexTop = lodMapY;
            uint32_t indexBottom = lodMapY;

            if(lodMapX > 0){
                indexLeft--;

                if(m_map.at(lodMapY).at(indexLeft).core > coreLOD){
                    m_map.at(lodMapY).at(lodMapX).left = 1;
                }else{
                    m_map.at(lodMapY).at(lodMapX).left = 0;
                }
            }

            if(lodMapX < terrain->patchesX - 1){
                indexRight++;

                if(m_map.at(lodMapY).at(indexRight).core > coreLOD){
                    m_map.at(lodMapY).at(lodMapX).right = 1;
                }else{
                    m_map.at(lodMapY).at(lodMapX).right = 0;
                }
            }

            if(lodMapY > 0){
                indexBottom--;

                if(m_map.at(indexBottom).at(lodMapX).core > coreLOD){
                    m_map.at(lodMapY).at(lodMapX).bottom = 1;
                }else{
                    m_map.at(lodMapY).at(lodMapX).bottom = 0;
                }
            }

            if(lodMapY < terrain->patchesY - 1){
                indexTop++;

                if(m_map.at(indexTop).at(lodMapX).core > coreLOD){
                    m_map.at(lodMapY).at(lodMapX).top = 1;
                }else{
                    m_map.at(lodMapY).at(lodMapX).top = 0;
                }
            }
        }
    }
}

uint32_t LODManager::distanceToLOD(float distance) {
    uint32_t LOD = terrain->maxLOD;
    for(uint32_t i = 0; i <= terrain->maxLOD; i++){
        if(distance < m_regions.at(i)){
            LOD = i;
            break;
        }
    }
    return LOD;
}

void LODManager::loadHeightsPerPatch(const std::vector<std::vector<float>>& map) {
    m_heights = map;
}
