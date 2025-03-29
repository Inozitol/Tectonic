#include "OBB.h"

void OBB::rotatePoint(glm::vec3 &p) const {
    p = rotation * p;
}

void OBB::rotatePoints(std::span<glm::vec3> px) const {
    for(auto& p : px) {
        p = rotation * p;
    }
}

void OBB::alignPoint(glm::vec3 &p) const {
    p = rotation * p;
    p += center;
}

void OBB::alignPoints(std::span<glm::vec3> px) const {
    for(auto& p : px) {
        p = rotation * p;
        p += center;
    }
}