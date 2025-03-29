#include "PatchManager.h"
#include "Terrain.h"

void PatchManager::init(Terrain *t) {
    assert(t);
    terrain = t;
    calcPatchBounds();
}

void PatchManager::calcPatchBounds() {
    glm::vec3 &p = terrain->pMapAt(0, 0);
    globPatchBounds = {p.x, p.z};
}

bool PatchManager::PatchBounds::isCoordInside(float x, float y) const { return x <= xPos && x >= xNeg && y <= yPos && y >= yNeg; }

[[nodiscard]] std::optional<PatchManager::PatchBounds> PatchManager::searchPatchBound(float x, float y) const {
    glm::vec3& limitingPoint = terrain->pMapAt(terrain->dimX-1, terrain->dimY-1);
    if(x < globPatchBounds.x || y < globPatchBounds.y || x > limitingPoint.x || y > limitingPoint.z) return {};

    float patchStep = static_cast<float>(terrain->patchSize - 1) * terrain->worldScale;

    float normX = x - globPatchBounds.x;
    float normY = y - globPatchBounds.y;

    uint32_t patchX = std::floor(normX / patchStep);
    uint32_t patchY = std::floor(normY / patchStep);

    return PatchBounds{
            .xCoord = patchX,
            .yCoord = patchY,
            .xPos = static_cast<float>(patchX + 1) * patchStep + globPatchBounds.x,
            .xNeg = static_cast<float>(patchX) * patchStep  + globPatchBounds.x,
            .yPos = static_cast<float>(patchY + 1) * patchStep  + globPatchBounds.y,
            .yNeg = static_cast<float>(patchY) * patchStep + globPatchBounds.y
    };
}

float PatchManager::getHeightAt(float x, float y) const {
    glm::vec3& limitingPoint = terrain->pMapAt(terrain->dimX-1, terrain->dimY-1);
    if(x < globPatchBounds.x || y < globPatchBounds.y || x > limitingPoint.x || y > limitingPoint.z) return -100;  // TODO this is just a placeholder value

    float fanStep = terrain->pMapAt(2,0).x - terrain->pMapAt(0,0).x;

    float normX = x - globPatchBounds.x;
    float normY = y - globPatchBounds.y;

    uint32_t fanX = std::floor(normX / fanStep);
    uint32_t fanY = std::floor(normY / fanStep);

    uint32_t globX = fanX*2+1;
    uint32_t globY = fanY*2+1;

    glm::vec3& fan0 = terrain->pMapAt(globX, globY);
    glm::vec3 fan1, fan2;
    if(x <= fan0.x) {
        if(y <= fan0.z) {
            fan1 = terrain->pMapAt(globX-1, globY-1);
            if(Utils::lineSide(fan0.x, fan0.z, fan1.x, fan1.z, x, y) <= 0) fan2 = terrain->pMapAt(globX-1, globY);
            else fan2 = terrain->pMapAt(globX, globY-1);
        }else{
            fan1 = terrain->pMapAt(globX-1, globY+1);
            if(Utils::lineSide(fan0.x, fan0.z, fan1.x, fan1.z, x, y) <= 0) fan2 = terrain->pMapAt(globX, globY+1);
            else fan2 = terrain->pMapAt(globX-1, globY);
        }
    }else {
        if(y <= fan0.z) {
            fan1 = terrain->pMapAt(globX+1, globY+1);
            if(Utils::lineSide(fan0.x, fan0.z, fan1.x, fan1.z, x, y) <= 0) fan2 = terrain->pMapAt(globX+1, globY);
            else fan2 = terrain->pMapAt(globX, globY+1);
        }else{
            fan1 = terrain->pMapAt(globX+1, globY-1);
            if(Utils::lineSide(fan0.x, fan0.z, fan1.x, fan1.z, x, y) <= 0) fan2 = terrain->pMapAt(globX, globY-1);
            else fan2 = terrain->pMapAt(globX+1, globY);
        }
    }
    float u,v,w;
    Utils::barycentric({x,y}, {fan0.x, fan0.z}, {fan1.x, fan1.z}, {fan2.x, fan2.z}, u,v,w);
    return fan0.y*u + fan1.y*v + fan2.y*w;
}
