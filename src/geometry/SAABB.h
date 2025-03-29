#pragma once
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "BB.h"

/**
 * Single Axis Aligned Bounding Box
 */
struct SAABB {
    /** Center point of the box */
    glm::vec3 center = {0.0f, 0.0f, 0.0f};

    /** Half length vectors, each corresponding to its axis, therefore:
     *  X - left/right
     *  Y - up/down
     *  Z - front/back
     */
    glm::vec3 halfLengths = {0.0f, 0.0f, 0.0f};

    /** Radians with rotation around an axis. */
    float rotation = 0.0f;

    /** Axis of the bounding box */
    BB::Axis axis = BB::Axis::X;

    void rotatePoint(glm::vec3 &p) const;
    void rotatePoints(std::span<glm::vec3> px) const;
    void alignPoint(glm::vec3 &p) const;
    void alignPoints(std::span<glm::vec3> px) const;
    [[nodiscard]] BB::Corners_t getCorners() const;
    [[nodiscard]] BB::SideCorners_t getSideCorners(BB::Side s) const;
    [[nodiscard]] float getSideAxisScalar(BB::Side s) const;
    void setSideAxisScalar(BB::Side s, float val);
};