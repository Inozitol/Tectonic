#ifndef TECTONIC_UTILS_H
#define TECTONIC_UTILS_H

#include <bitset>
#include <cstring>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#include "connector/Signal.h"
#include "connector/Slot.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define INIT_ENUM_FR_OR(EnumClass) \
    friend inline EnumClass operator|(EnumClass lhs, EnumClass rhs){ \
        return Utils::enumFromVal<EnumClass>(Utils::enumVal(lhs) | Utils::enumVal(rhs)); \
    } \
    friend inline EnumClass& operator |=(EnumClass& lhs, EnumClass rhs){\
        return lhs = lhs | rhs; \
    }

#define INIT_ENUM_FR_AND(EnumClass) \
    friend inline bool operator&(EnumClass lhs, EnumClass rhs){ \
        return (Utils::enumVal(lhs) & Utils::enumVal(rhs)) == Utils::enumVal(rhs); \
    }

#define INIT_ENUM_FR_OP(EnumClass) \
    INIT_ENUM_FR_OR(EnumClass)\
    INIT_ENUM_FR_AND(EnumClass)

#define INIT_ENUM_OR(EnumClass) \
    inline EnumClass operator|(EnumClass lhs, EnumClass rhs){ \
        return Utils::enumFromVal<EnumClass>(Utils::enumVal(lhs) | Utils::enumVal(rhs)); \
    } \
    inline EnumClass& operator |=(EnumClass& lhs, EnumClass rhs){\
        return lhs = lhs | rhs; \
    }


#define INIT_ENUM_AND(EnumClass) \
    inline bool operator&(EnumClass lhs, EnumClass rhs){ \
        return (Utils::enumVal(lhs) & Utils::enumVal(rhs)) == Utils::enumVal(rhs); \
    }

#define INIT_ENUM_OP(EnumClass) \
    INIT_ENUM_OR(EnumClass)\
    INIT_ENUM_AND(EnumClass)

namespace Utils {
    struct WindowDimension {
        uint32_t width = 0;
        uint32_t height = 0;

        WindowDimension(uint32_t width, uint32_t height);
        WindowDimension(int32_t width, int32_t height);

        [[nodiscard]] float ratio() const;
    };

    bool readFile(const char* filename, std::string& content);

    template <typename Enum_t>
    class Flags {
        static_assert(std::is_enum_v<Enum_t>, "Flags supports only enum types");
        using EnumU_t = typename std::make_unsigned_t<typename std::underlying_type<Enum_t>::type>;

    public:
        Flags& set(Enum_t flag, bool val = true) {
            if (m_bits[underlying(flag)] != val) {
                sig_flagChanged.emit(flag, val);
            }
            m_bits.set(underlying(flag), val);
            return *this;
        }

        constexpr bool operator[](Enum_t flag) const {
            return m_bits[underlying(flag)];
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return m_bits.size();
        }

        Signal<Enum_t, bool> sig_flagChanged;

    private:
        static constexpr EnumU_t underlying(Enum_t flag) {
            return static_cast<EnumU_t>(flag);
        }

        std::bitset<underlying(Enum_t::MAX)> m_bits;
    };

    template <typename E>
    concept Enumerated = std::is_enum_v<E>;

    /**
     * Converts any enum value into underlying type.
     * It will return identity if the type isn't constrained by is_enum.
     * @tparam E Enum type
     * @param val Enum value
     * @return underlying_type_t<T>
     */
    template <Enumerated E>
    constexpr auto enumVal(E const val) -> std::underlying_type_t<E> {
        return static_cast<std::underlying_type_t<E>>(val);
    }

    template <typename E>
    constexpr E enumVal(E const val) {
        return val;
    }

    template <Enumerated E, typename V>
    constexpr E enumFromVal(V const val) {
        return static_cast<E>(val);
    }

    template <Enumerated E>
    constexpr E enumFromVal(E const val) {
        return val;
    }


    /**
     * Sets bits present in val into current and returns a new value.
     * @tparam T1 Numeric or Enum type
     * @param current Current bits
     * @param bits Bits to add
     */
    template <typename T1>
    void enumSetBits(T1& current, T1 bits) {
        current = static_cast<T1>(enumVal<T1>(bits) | enumVal<T1>(current));
    }

    /** Checks whether a bit of underlying enum value in comp is present in val */
    template <typename T1, typename T2>
    bool enumCheckBit(const T1 val, const T2 comp) {
        return (enumVal<T1>(val) & enumVal(comp)) == enumVal(comp);
    }

    /**
     * @brief Returns a distance from two transformations
     * @param s1 First scale vector
     * @param r1 First rotation quaternion
     * @param t1 First translation vector
     * @param mat1 First world matrix
     * @param s2 Second scale vector
     * @param r2 Second rotation quaternion
     * @param t2 Second translation vector
     * @param mat2 Second world matrix
     * @return Distance from two transformations
     */
    float transformDistance(glm::vec3 s1, glm::quat r1, glm::vec3 t1, glm::mat4 mat1, glm::vec3 s2, glm::quat r2, glm::vec3 t2, glm::mat4 mat2);

    /**
     * @brief Returns a distance from transformation and a position
     * @param s1 First scale vector
     * @param r1 First rotation quaternion
     * @param t1 First translation vector
     * @param mat1 First world matrix
     * @param pos2 Second position
     * @return Distance from two transformations
     */
    float transformDistance(glm::vec3 s1, glm::quat r1, glm::vec3 t1, glm::mat4 mat1, glm::vec3 pos2);

    /**
     * @brief Returns a distance from two transformations
     * @param mat1 First transformation matrix
     * @param mat2 Second transformation matrix
     * @return Distance from two transformations
     */
    float transformDistance(glm::mat4 mat1, glm::mat4 mat2);

    /**
     * @brief Returns a point between two points interpolated by delta
     * @param pos1 First point
     * @param pos2 Second point
     * @param delta Delta interpolation in interval <0,1>
     * @return Interpolated point between pos1 and pos2
     */
    glm::vec3 interpolateBetween(glm::vec3 pos1, glm::vec3 pos2, float delta);

    glm::vec3 closestOrthonormal(const glm::vec3& base, const glm::vec3& target);

    bool isRightHanded(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z);

    float ellipse(float x, float y, float a, float b);

    /*
    class Frustum{
    public:
        Frustum() = default;

        void calcCorners(const Camera& camera);

        glm::vec4 nearCenter;

        glm::vec4 nearTopLeft;
        glm::vec4 nearTopRight;
        glm::vec4 nearBottomLeft;
        glm::vec4 nearBottomRight;

        glm::vec4 farTopLeft;
        glm::vec4 farTopRight;
        glm::vec4 farBottomLeft;
        glm::vec4 farBottomRight;
    };
*/
    struct FrustumCulling {
        void update(const glm::mat4& VP);
        [[nodiscard]] bool isPointInside(const glm::vec3& point) const;

        Slot<const glm::mat4&> slt_updateVP{[this](const glm::mat4& VP) { update(VP); }};

        float bias = 0.0;

        glm::vec4 leftClipPlane{};
        glm::vec4 rightClipPlane{};
        glm::vec4 bottomClipPlane{};
        glm::vec4 topClipPlane{};
        glm::vec4 nearClipPlane{};
        glm::vec4 farClipPlane{};
    };

    /*
        OrthoProjInfo createTightOrthographicInfo(Camera &lightCamera, const Camera &gameCamera);
    */

    template <typename Integer, typename Power>
    constexpr Integer nextPowerOf(Integer in, Power power) {
        static_assert(std::is_integral_v<Integer> && std::is_integral_v<Power>, "Requires integer types.");
        return static_cast<Integer>(std::pow(power, static_cast<Integer>(std::ceil(std::log(in) / std::log(power)))));
    }

    template <typename Integer>
    constexpr Integer binPow(Integer exp) {
        static_assert(std::is_integral_v<Integer>, "Requires integer type.");
        return 1 << exp;
    }

    template <typename Integer>
    constexpr Integer halfPoint(Integer left, Integer right) {
        static_assert(std::is_integral_v<Integer>, "Requires integer type.");
        return left + (right - left) / 2;
    }

    template <typename Unsigned>
    constexpr Unsigned unsignedDst(Unsigned a, Unsigned b) {
        static_assert(std::is_unsigned_v<Unsigned> && std::is_arithmetic_v<Unsigned>, "Requires unsigned arithmetic type.");
        return a >= b ? a - b : b - a;
    }

    /**
     * Given a line from (x0,x1) to (y0,y1) and a point (z0,z1),
     * the returned value is positive if that point lies on the right side of the line,
     * given a perspective from the line origin to destination.
     */
    template <typename Number>
    constexpr Number lineSide(Number x0, Number x1, Number y0, Number y1, Number z0, Number z1) {
        static_assert(std::is_arithmetic_v<Number>, "Requires arithmetic type.");
        return (z0 - x0) * (y1 - x1) - (z1 - x1) * (y0 - x0);
    }

    void barycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c, float& u, float& v, float& w);
} // namespace Utils

#endif//TECTONIC_UTILS_H
