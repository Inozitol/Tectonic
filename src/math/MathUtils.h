#pragma once
#include <glm/fwd.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MathUtils {
    inline glm::quat quatGetXRot(const glm::quat& quat) {
        glm::quat q = quat;
        q.y = 0;
        q.z = 0;
        return glm::normalize(q);
    }

    inline glm::quat quatGetYRot(const glm::quat& quat) {
        glm::quat q = quat;
        q.x = 0.0f;
        q.z = 0.0f;
        return glm::normalize(q);
    }

    inline glm::quat quatGetZRot(const glm::quat& quat) {
        glm::quat q = quat;
        q.x = 0;
        q.y = 0;
        return glm::normalize(q);
    }

}