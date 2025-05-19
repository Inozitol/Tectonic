#include "AABB.h"

#include <utility>

void AABB::alignPoint(glm::vec3 &p) const { p += center; }
void AABB::alignPoints(std::span<glm::vec3> px) const { for(auto &p: px) { p += center; } }

BB::Corners_t AABB::getCorners() const {
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

BB::SideCorners_t AABB::getSideCorners(const BB::Side s) const {
    const glm::vec3 &hl = halfLengths;
    BB::SideCorners_t side;
    switch(s) {
        case BB::Side::XPOS: {
            side = {
                    glm::vec3{+hl.x, +hl.y, -hl.z},
                    glm::vec3{+hl.x, +hl.y, +hl.z},
                    glm::vec3{+hl.x, -hl.y, +hl.z},
                    glm::vec3{+hl.x, -hl.y, -hl.z},
            };
        }
        break;
        case BB::Side::XNEG: {
            side = {
                    glm::vec3{-hl.x, +hl.y, -hl.z},
                    glm::vec3{-hl.x, +hl.y, +hl.z},
                    glm::vec3{-hl.x, -hl.y, +hl.z},
                    glm::vec3{-hl.x, -hl.y, -hl.z},
            };
        }
        break;
        case BB::Side::YPOS: {
            side = {
                    glm::vec3{+hl.x, +hl.y, -hl.z},
                    glm::vec3{+hl.x, +hl.y, +hl.z},
                    glm::vec3{-hl.x, +hl.y, +hl.z},
                    glm::vec3{-hl.x, +hl.y, -hl.z},
            };
        }
        break;
        case BB::Side::YNEG: {
            side = {
                    glm::vec3{+hl.x, -hl.y, -hl.z},
                    glm::vec3{+hl.x, -hl.y, +hl.z},
                    glm::vec3{-hl.x, -hl.y, +hl.z},
                    glm::vec3{-hl.x, -hl.y, -hl.z},
            };
        }
        break;
        case BB::Side::ZPOS: {
            side = {
                    glm::vec3{-hl.x, +hl.y, +hl.z},
                    glm::vec3{+hl.x, +hl.y, +hl.z},
                    glm::vec3{+hl.x, -hl.y, +hl.z},
                    glm::vec3{-hl.x, -hl.y, +hl.z},
            };
        }
        break;
        case BB::Side::ZNEG: {
            side = {
                    glm::vec3{+hl.x, +hl.y, -hl.z},
                    glm::vec3{-hl.x, +hl.y, -hl.z},
                    glm::vec3{-hl.x, -hl.y, -hl.z},
                    glm::vec3{+hl.x, -hl.y, -hl.z},
            };
        }
        break;
        default:
            std::unreachable();
    }
    alignPoints(side);
    return side;
}
