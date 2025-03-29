#include "SAABB.h"

#include <utility>

void SAABB::rotatePoint(glm::vec3 &p) const {
    switch(axis) {
        case BB::Axis::X:
            p = glm::rotateX(p, rotation);
            break;

        case BB::Axis::Y:
            p = glm::rotateY(p, rotation);
            break;

        case BB::Axis::Z:
            p = glm::rotateZ(p, rotation);
            break;
        default:
            std::unreachable();
    }
}

void SAABB::rotatePoints(std::span<glm::vec3> px) const {
    switch(axis) {
        case BB::Axis::X:
            for(auto &p: px) { p = glm::rotateX(p, rotation); }
            break;

        case BB::Axis::Y:
            for(auto &p: px) { p = glm::rotateY(p, rotation); }
            break;

        case BB::Axis::Z:
            for(auto &p: px) { p = glm::rotateZ(p, rotation); }
            break;
        default:
            std::unreachable();
    }
}

void SAABB::alignPoint(glm::vec3 &p) const {
    switch(axis) {
        case BB::Axis::X:
            p = glm::rotateX(p, rotation);
            break;

        case BB::Axis::Y:
            p = glm::rotateY(p, rotation);
            break;

        case BB::Axis::Z:
            p = glm::rotateZ(p, rotation);
            break;
        default:
            std::unreachable();
    }
    p += center;
}

void SAABB::alignPoints(std::span<glm::vec3> px) const {
    switch(axis) {
        case BB::Axis::X:
            for(auto &p: px) {
                p = glm::rotateX(p, rotation);
                p += center;
            }
            break;

        case BB::Axis::Y:
            for(auto &p: px) {
                p = glm::rotateY(p, rotation);
                p += center;
            }
            break;

        case BB::Axis::Z:
            for(auto &p: px) {
                p = glm::rotateZ(p, rotation);
                p += center;
            }
            break;
        default:
            std::unreachable();
    }
}

BB::Corners_t SAABB::getCorners() const {
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

BB::SideCorners_t SAABB::getSideCorners(const BB::Side s) const {
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

float SAABB::getSideAxisScalar(const BB::Side s) const {
    glm::vec3 sidePoint;
    switch(s) {
        case BB::Side::XPOS:
            sidePoint = center + halfLengths.x;
            rotatePoint(sidePoint);
            return sidePoint.x;

        case BB::Side::XNEG:
            sidePoint = center - halfLengths.x;
            rotatePoint(sidePoint);
            return sidePoint.x;

        case BB::Side::YPOS:
            sidePoint = center + halfLengths.y;
            rotatePoint(sidePoint);
            return sidePoint.y;

        case BB::Side::YNEG:
            sidePoint = center - halfLengths.y;
            rotatePoint(sidePoint);
            return sidePoint.y;

        case BB::Side::ZPOS:
            sidePoint = center + halfLengths.z;
            rotatePoint(sidePoint);
            return sidePoint.z;

        case BB::Side::ZNEG:
            sidePoint = center - halfLengths.z;
            rotatePoint(sidePoint);
            return sidePoint.z;

        default:
            std::unreachable();
    }
}

void SAABB::setSideAxisScalar(const BB::Side s, const float val) {
    switch(s) {
        case BB::Side::XPOS:
            assert(axis == BB::Axis::X);
        center.x = val - halfLengths.x;
        break;

        case BB::Side::XNEG:
            assert(axis == BB::Axis::X);
        center.x = val + halfLengths.x;
        break;

        case BB::Side::YPOS:
            assert(axis == BB::Axis::Y);
        center.y = val - halfLengths.y;
        break;

        case BB::Side::YNEG:
            assert(axis == BB::Axis::Y);
        center.y = val + halfLengths.y;
        break;

        case BB::Side::ZPOS:
            assert(axis == BB::Axis::Z);
        center.z = val - halfLengths.z;
        break;

        case BB::Side::ZNEG:
            assert(axis == BB::Axis::Z);
        center.z = val + halfLengths.z;
        break;

        default:
            std::unreachable();
    }
}