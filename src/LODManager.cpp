#include "model/terrain/LODManager.h"

Logger LODManager::m_logger = Logger("LOD Manager");

void LODManager::init(uint32_t maxLOD, uint32_t patchesX, uint32_t patchesY, float scale) {
    m_worldScale = scale;

    m_patchSize = Utils::binPow(static_cast<int32_t>(maxLOD+1)) + 1;
    m_maxLOD = maxLOD;
    m_patchesX = patchesX;
    m_patchesY = patchesY;
    m_dimX = m_patchesX * (m_patchSize-1) + 1;
    m_dimY = m_patchesY * (m_patchSize-1) + 1;

    m_map.resize(m_patchesY);
    for(uint32_t y = 0; y < patchesY; y++){
        m_map.at(y).resize(m_patchesX);
    }

    m_regions.resize(m_maxLOD+1);
    calcLODRegions();
}

void LODManager::calcLODRegions() {
    uint32_t sum = 0;

    for(uint32_t i = 0; i <= m_maxLOD; i++){
        sum += (i + 1);
    }

    float X = CAMERA_PPROJ_FAR / static_cast<float>(sum);

    float temp = 0;

    for(uint32_t i = 0; i <= m_maxLOD; i++){
        auto currRange = X * static_cast<float>(i+1);
        m_regions.at(i) = temp + currRange;
        temp += currRange;
        m_logger(Logger::DEBUG) << "Regions with LOD " << i << " set to range " << currRange << '\n';
    }
}

const LODManager::patchLOD &LODManager::getPatchLOD(uint32_t patchX, uint32_t patchY) const {
    return m_map.at(patchY).at(patchX);
}

void LODManager::updateLODMapPass1(const glm::vec3 &camPos) {
    int32_t centerStep = m_patchSize / 2;
    for(uint32_t lodMapY = 0; lodMapY < m_patchesY; lodMapY++){
        for(uint32_t lodMapX = 0; lodMapX < m_patchesX; lodMapX++) {
            uint32_t x = lodMapX * (m_patchSize - 1) + centerStep;
            uint32_t y = lodMapY * (m_patchSize - 1) + centerStep;

            glm::vec3 patchCenter = glm::vec3((static_cast<float>(x) - static_cast<float>(m_dimX)/2) * m_worldScale,
                                              m_heights.at(lodMapY).at(lodMapX),
                                              (static_cast<float>(y) - static_cast<float>(m_dimY)/2) * m_worldScale);

            float distanceToCamera = glm::distance(patchCenter, camPos);

            uint32_t coreLOD = distanceToLOD(distanceToCamera);

            patchLOD* pPatchLOD = &m_map.at(lodMapY).at(lodMapX);
            pPatchLOD->core = coreLOD;
        }
    }
}

void LODManager::updateLODMapPass2(const glm::vec3 &camPos) {

    for(uint32_t lodMapY = 0; lodMapY < m_patchesY; lodMapY++) {
        for (uint32_t lodMapX = 0; lodMapX < m_patchesX; lodMapX++) {
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

            if(lodMapX < m_patchesX - 1){
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

            if(lodMapY < m_patchesY - 1){
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
    uint32_t LOD = m_maxLOD;
    for(uint32_t i = 0; i <= m_maxLOD; i++){
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
