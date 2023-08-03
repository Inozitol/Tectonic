#ifndef TECTONIC_UTILS_H
#define TECTONIC_UTILS_H

#include <fstream>
#include <iostream>
#include <cstring>
#include <glm/vec4.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>

#include "extern/glad/glad.h"
#include "camera/Camera.h"

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
    OrthoProjInfo createTightOrthographicInfo(Camera &lightCamera, const Camera &gameCamera);
    bool readFile(const char* filename, std::string& content);
    glm::mat4 aiMatToGLM(const aiMatrix4x4t<ai_real>& mat);
    glm::vec3 aiVecToGLM(const aiVector3D& vec);
    glm::quat aiQuatToGLM(const aiQuaternion& orientation);
}

#endif //TECTONIC_UTILS_H
