#ifndef TECTONIC_UTILS_H
#define TECTONIC_UTILS_H

#include <fstream>
#include <iostream>
#include <cstring>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <assimp/types.h>
#include <bitset>

#include "extern/glad/glad.h"
#include "camera/Camera.h"
#include "meta/meta.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define INVALID_UNIFORM_LOC 0xFFFFFFFF

namespace Utils{
    /**
     * @brief Class for binding a VAO by creating an object on stack.
     */
    class EphemeralVAOBind{
    public:
        explicit EphemeralVAOBind(GLuint vao){ glBindVertexArray(vao); }
        ~EphemeralVAOBind(){ glBindVertexArray(0); }
    };

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
        /**
         * @param bias Bias for frustum planes checking. Should be positive.
         */
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

    OrthoProjInfo createTightOrthographicInfo(Camera &lightCamera, const Camera &gameCamera);
    bool readFile(const char* filename, std::string& content);
    uint32_t nextPowerOf(uint32_t in, uint32_t power);

    int64_t binPow(int32_t exp);

    void barycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c, float &u, float &v, float &w);
}

#endif //TECTONIC_UTILS_H
