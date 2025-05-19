#include "OBB.h"

void OBB::rotatePoint(glm::vec3 &p) const { p = rotation * p; }

void OBB::rotatePoints(std::span<glm::vec3> px) const { for(auto &p: px) { p = rotation * p; } }

void OBB::alignPoint(glm::vec3 &p) const {
    p = rotation * p;
    p += center;
}

void OBB::alignPoints(std::span<glm::vec3> px) const {
    for(auto &p: px) {
        p = rotation * p;
        p += center;
    }
}

BB::Corners_t OBB::getCorners() const {
    const glm::vec3 &hl = halfLengths;
    BB::Corners_t points{
            glm::vec3{+hl.x, +hl.y, -hl.z},
            glm::vec3{+hl.x, +hl.y, +hl.z},
            glm::vec3{-hl.x, +hl.y, -hl.z},
            glm::vec3{-hl.x, +hl.y, +hl.z},
            glm::vec3{+hl.x, -hl.y, -hl.z},
            glm::vec3{+hl.x, -hl.y, +hl.z},
            glm::vec3{-hl.x, -hl.y, -hl.z},
            glm::vec3{-hl.x, -hl.y, +hl.z},
    };
    alignPoints(points);
    return points;
}