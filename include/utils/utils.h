#ifndef TECTONIC_UTILS_H
#define TECTONIC_UTILS_H

#include <fstream>
#include <iostream>
#include <cstring>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <bitset>

//#include "camera/Camera.h"
#include "meta/meta.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define INVALID_UNIFORM_LOC 0xFFFFFFFF

namespace Utils{
    struct WindowDimension{
        uint32_t width = 0;
        uint32_t height = 0;

        WindowDimension(uint32_t width, uint32_t height);
        WindowDimension(int32_t width, int32_t height);

        [[nodiscard]] float ratio() const;
    };

    bool readFile(const char* filename, std::string& content);

    template<typename Enum_t>
    class Flags{
        static_assert(std::is_enum_v<Enum_t>, "Flags class supports only enum types");
        using EnumU_t = typename std::make_unsigned_t<typename std::underlying_type<Enum_t>::type>;
    public:
        Flags& set(Enum_t flag, bool val = true) {
            if(m_bits[underlying(flag)] != val) {
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
        static constexpr EnumU_t underlying(Enum_t flag){
            return static_cast<EnumU_t>(flag);
        }
        std::bitset<underlying(Enum_t::SIZE)> m_bits;
    };

    template<typename E>
    concept Enumerated = std::is_enum_v<E>;

    /**
     * Converts any enum value into underlying type.
     * It will return identity if the type isn't constrained by is_enum.
     * @tparam E Enum type
     * @param val Enum value
     * @return underlying_type_t<T>
     */
    template<Enumerated E>
    auto enumVal(E const val) -> typename std::underlying_type_t<E>{
        return static_cast<typename std::underlying_type_t<E>>(val);
    }

    template<typename E>
    E enumVal(E const val){
        return val;
    }

    /**
     * Sets bits present in val into current and returns a new value.
     * @tparam T1 Numeric or Enum type
     * @tparam T2 Numeric or Enum type
     * @param val Bits to add
     * @param current Current bits
     * @return Current with new bits
     */
    template<typename T1, typename T2>
    T2 enumSetBits(const T1 val, const T2 current){
        return static_cast<T2>((enumVal(val) | enumVal(current)));
    }

    /** Checks whether a bit of underlying enum value in comp is present in val */
    template<typename T1, typename T2>
    bool enumCheckBit(const T1 val, const T2 comp){
        return (enumVal(val) & enumVal(comp)) == enumVal(comp);
    }

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

    class FrustumCulling{
    public:
        FrustumCulling(float bias) : m_bias(-bias){}

        void update(const glm::mat4& VP);
        [[nodiscard]] bool isPointInside(const glm::vec3& point) const;

        Slot<const glm::mat4&> slt_updateVP{[this](const glm::mat4& VP) { update(VP); }};
    private:

        float m_bias = 0.0;

        glm::vec4 m_leftClipPlane{};
        glm::vec4 m_rightClipPlane{};
        glm::vec4 m_bottomClipPlane{};
        glm::vec4 m_topClipPlane{};
        glm::vec4 m_nearClipPlane{};
        glm::vec4 m_farClipPlane{};
    };


    OrthoProjInfo createTightOrthographicInfo(Camera &lightCamera, const Camera &gameCamera);
    uint32_t nextPowerOf(uint32_t in, uint32_t power);

    int64_t binPow(int32_t exp);

    void barycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c, float &u, float &v, float &w);
    */
}

#endif //TECTONIC_UTILS_H
