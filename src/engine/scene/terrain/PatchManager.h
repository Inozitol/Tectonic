#pragma once
#include <cstdint>
#include <iostream>
#include "utils/Logger.h"
#include <vector>
#include "connector/Slot.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "engine/GlobalMemory.h"

constexpr std::size_t t = sizeof(float);

struct Terrain;

struct PatchManager {
    void init(Terrain* terrain);

    /** This contains the lowest X,Y bounds of the map */
    glm::vec2 globPatchBounds;

    struct PatchBounds {
        uint32_t xCoord = 0;
        uint32_t yCoord = 0;
        float xPos = 0.0f;
        float xNeg = 0.0f;
        float yPos = 0.0f;
        float yNeg = 0.0f;
        [[nodiscard]] bool isCoordInside(float x, float y) const;
    };

    bool setPatchCoord(PatchBounds& bounds, uint32_t x, uint32_t y) const;

    [[nodiscard]] std::optional<PatchBounds> searchPatchBound(float x, float y) const;

    void calcPatchBounds();

    float getHeightAt(float x, float y) const;

    Terrain* terrain;

    PatchBounds cameraPatch{};

    Signal<uint32_t, uint32_t> sig_patchBoundsChange;

    Slot<const glm::vec3&> slt_cameraPosition{
        [this](const glm::vec3& pos) {
            if(!cameraPatch.isCoordInside(pos.x, pos.z)) {
                auto newPatch = searchPatchBound(pos.x, pos.z);
                if(!newPatch.has_value()) {
                    LOG(LOG_DEBUG, "Went out of bounds");
                    return;
                }
                cameraPatch = newPatch.value();
                LOG(LOG_DEBUG, "New patch coordinates: " << cameraPatch.xCoord << " | " << cameraPatch.yCoord <<
                               " with bounds at xPos: " << cameraPatch.xPos << " xNeg: " << cameraPatch.xNeg <<
                                              " yPos: " << cameraPatch.yPos << " yNeg: " << cameraPatch.yNeg );
                sig_patchBoundsChange.emit(cameraPatch.xCoord, cameraPatch.yCoord);
            }
        }
    };

};
