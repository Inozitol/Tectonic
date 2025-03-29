#pragma once
#include <array>
#include <cstdint>
#include <utils/Utils.h>

// Taken from: https://learncheme.com/simulations/fluid-mechanics/drag-force/
// plus some custom additions

namespace Physics {
    enum class Object : uint8_t {
        HUMAN = 0,
        MAX
    };
    static constexpr std::array DRAG_COEFS = {
        1.2f // HUMAN
    };

    static constexpr std::array CROSS_SECTION_AREAS = {
        1.0f // HUMAN
    };

    static constexpr float AIR_DENSITY = 1.23f; // kg/m^3
    static constexpr float GRAV_ACC = 2.0f; // m/s^2
    static constexpr float FALLOFF = 50.0f;

    template<Object O>
    constexpr float velocityFall(const float mass, const float t) {
        constexpr float alpha_top = CROSS_SECTION_AREAS[Utils::enumVal(O)]*DRAG_COEFS[Utils::enumVal(O)]*AIR_DENSITY;
        const float alpha = alpha_top / (2*mass);
        return -std::tanh(std::sqrtf(alpha*GRAV_ACC)*t) * std::sqrt(GRAV_ACC/alpha);
    }

    template<Object O>
    constexpr float velocityJump(const float mass, const float jumpForce, const float t) {
        constexpr float alpha_top = CROSS_SECTION_AREAS[Utils::enumVal(O)]*DRAG_COEFS[Utils::enumVal(O)]*AIR_DENSITY;
        const float alpha = alpha_top / (2*mass);
        const float V = std::tanh(std::sqrtf(alpha*GRAV_ACC)*t) * std::sqrt(GRAV_ACC/alpha);
        const float J = std::expf(-t) * jumpForce * t;
        return J-V;
    }

}