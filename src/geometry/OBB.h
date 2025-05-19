#pragma once

#include "BB.h"

#include <glm/vec3.hpp>

/** Oriented Bounding Box */
struct OBB {
    /** Center point of the box */
    glm::vec3 center = {0.0f, 0.0f, 0.0f};

    /** Half length vectors, each corresponding to its axis, therefore:
     *  X - left/right
     *  Y - up/down
     *  Z - front/back
     */
    glm::vec3 halfLengths = {0.0f, 0.0f, 0.0f};

    /** Radians with rotation around an axis. */
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};

    void rotatePoint(glm::vec3 &p) const;
    void rotatePoints(std::span<glm::vec3> px) const;
    void alignPoint(glm::vec3 &p) const;
    void alignPoints(std::span<glm::vec3> px) const;
    [[nodiscard]] BB::Corners_t getCorners() const;
};
