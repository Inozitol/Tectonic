#ifndef TECTONIC_CAMERA_H
#define TECTONIC_CAMERA_H

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>
#include <memory>

#include "meta/meta.h"

#define NUM_CUBE_MAP_FACES 6

/**
 * Various constant normalized vectors of axis used in camera calculations.
 */
namespace Axis{
    static constexpr glm::vec3 POS_X(1.0f,0.0f,0.0f);
    static constexpr glm::vec3 POS_Y(0.0f,1.0f,0.0f);
    static constexpr glm::vec3 POS_Z(0.0f,0.0f,1.0f);
    static constexpr glm::vec3 NEG_X(-1.0f,0.0f,0.0f);
    static constexpr glm::vec3 NEG_Y(0.0f,-1.0f,0.0f);
    static constexpr glm::vec3 NEG_Z(0.0f,0.0f,-1.0f);
}

struct CameraDirection{
    GLenum cubemapFace;
    glm::vec3 target;
    glm::vec3 up;
};

constexpr CameraDirection g_cameraDirections[NUM_CUBE_MAP_FACES]{
    {GL_TEXTURE_CUBE_MAP_POSITIVE_X, Axis::POS_X, Axis::POS_Y},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_X, Axis::NEG_X, Axis::POS_Y},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Y, Axis::POS_Y, Axis::NEG_Z},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, Axis::NEG_Y, Axis::POS_Z},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Z, Axis::NEG_Z, Axis::POS_Y},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, Axis::POS_Z, Axis::POS_Y},
};

/**
 * Holds information needed to create a perspective projection matrix.
 * Used as an input argument to a Camera object.
 */
struct PerspProjInfo{
    float fov;
    float aspect;
    float zNear;
    float zFar;
};

/**
 * Holds information needed to create a orthogonal projection matrix.
 * Used as an input argument to a Camera object.
 */
struct OrthoProjInfo{
    float left;
    float right;
    float bottom;
    float top;
    float zNear;
    float zFar;
};

/**
 * Represents a general camera positioned in the world space.
 * Provides a view and projection matrix.
 * Can be used as a view point from a spot light for shader mapping.
 */
class Camera {
public:
    Camera() = default;

    Camera(const Camera& camera);

    ~Camera();

    void setPerspectiveInfo(const PerspProjInfo& info);
    void setOrthographicInfo(const OrthoProjInfo& info);

    const PerspProjInfo& getPerspectiveInfo() const;
    const OrthoProjInfo& getOrthographicInfo() const;

    /**
     * Projection matrix is unchanged until the change of parameters in PerspProjInfo.
     * In case of change in those parameters, it is necessary to call createProjectionMatrix before this method.
     *
     * @brief Returns a reference to projection matrix.
     * @return Reference to generated projection matrix.
     */
    [[nodiscard]] const glm::mat4x4& getProjectionMatrix() const;

    /**
     * View matrix is changed with every change in orientation or getPosition.
     * In case of change in those parameters, it is necessary to call createView before this method.
     *
     * @brief Returns a reference to previously created view matrix.
     * @return Reference to generated projection matrix.
     */
    [[nodiscard]] const glm::mat4x4& getViewMatrix() const;

    glm::mat4 getWVP(const glm::mat4& modelTrans);

    glm::mat4 getVP();

    /**
     * Creates a projection matrix and stores it.
     * Should be called after a change to projection parameters.
     */
    void createProjectionMatrix();

    /**
     * Creates a view matrix and stores it.
     * Should be called after a change to orientation or local_position.
     */
    void createView();

    /**
     * @brief Returns a local_position of the camera in world space.
     * @return Reference to a local_position in worlds space.
     */
    [[nodiscard]] const glm::vec3& getPosition() const;

    /**
     * @brief Creates a normalized vector with the getDirection of the camera.
     * @return Normalized vector with the getDirection of the camera.
     */
    glm::vec3 getDirection() const;

    /**
     * Sets the local_position of the camera in world space.
     * @param position Vector with local_position in world space.
     */
    void setPosition(glm::vec3 position);

    /**
     * Sets the direction of the camera from a normalized getDirection vector.
     * @param direction Normalized direction vector.
     * @param up Normalized up vector of camera.
     */
    void setDirection(glm::vec3 direction, glm::vec3 up = Axis::POS_Y);

    void switchPerspective();
    void switchOrthographic();
    void toggleProjection();

    [[nodiscard]] glm::vec3 forward() const {return Axis::NEG_Z * m_orientation;}
    [[nodiscard]] glm::vec3 back()    const {return Axis::POS_Z * m_orientation;}
    [[nodiscard]] glm::vec3 left()    const {return Axis::NEG_X * m_orientation;}
    [[nodiscard]] glm::vec3 right()   const {return Axis::POS_X * m_orientation;}
    [[nodiscard]] glm::vec3 down()    const {return Axis::NEG_Y * m_orientation;}
    [[nodiscard]] glm::vec3 up()      const {return Axis::POS_Y * m_orientation;}

    Signal<glm::vec3> sig_position;

protected:
    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    glm::quat m_orientation = glm::quatLookAt(Axis::NEG_Z, Axis::POS_Y);
    glm::mat4 m_projectionMatrix = glm::identity<glm::mat4x4>();

private:
    inline glm::mat4x4 rotationMatrix(){
        return glm::toMat4(m_orientation);
    }

    inline glm::mat4x4 translationMatrix(){
        return glm::translate(glm::identity<glm::mat4>(), -m_position);
    }

    std::unique_ptr<PerspProjInfo> m_perspProjInfo;
    std::unique_ptr<OrthoProjInfo> m_orthoProjInfo;

    /**
     * Set to true if camera is in perspective projection and false if it's in orthographic projection.
     */
    bool isPerspective = true;

    glm::mat4 m_viewMatrix = glm::identity<glm::mat4x4>();
};

#endif //TECTONIC_CAMERA_H
