#pragma once

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <array>
#include <span>

namespace BB {

    enum class Axis : uint8_t {
        X, Y, Z
    };

    static constexpr std::size_t BOX_CORNERS = 8;
    static constexpr std::size_t BOX_SIDES = 6;
    static constexpr std::size_t BOX_TRIANGLES = BOX_SIDES * 2;
    static constexpr std::size_t BOX_INDICES = BOX_TRIANGLES * 3;
    static constexpr std::size_t BOX_SIDE_CORNERS = 4;

    enum class Side {
        XPOS,
        XNEG,
        YPOS,
        YNEG,
        ZPOS,
        ZNEG
    };

    using Corners_t = std::array<glm::vec3, BOX_CORNERS>;
    using SideCorners_t = std::array<glm::vec3, BOX_SIDE_CORNERS>;
}