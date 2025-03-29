#pragma once

#include "BB.h"

struct AABB {
    /** Center point of the box */
    glm::vec3 center = {0.0f, 0.0f, 0.0f};

    /** Half length vectors, each corresponding to its axis, therefore:
     *  X - left/right
     *  Y - up/down
     *  Z - front/back
     */
    glm::vec3 halfLengths = {0.0f, 0.0f, 0.0f};

    inline void alignPoint(glm::vec3 &p) const;
    inline void alignPoints(std::span<glm::vec3> px) const;
    [[nodiscard]] BB::Corners_t getCorners() const;
    [[nodiscard]] BB::SideCorners_t getSideCorners(BB::Side s) const;
};
