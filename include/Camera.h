#ifndef TECTONIC_CAMERA_H
#define TECTONIC_CAMERA_H

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>

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

/**
 * Holds information needed to create a projection matrix.
 * Used as an input argument to a Camera object.
 */
struct PersProjInfo{
    float fov;
    float ratio;
    float zNear;
    float zFar;
};

/**
 * Represents a general camera positioned in the world space.
 * Provides a view and projection matrix.
 * Can be used as a view point from a spot light for shadow mapping.
 */
class Camera {
public:
    /**
     * @brief Creates a Camera object.
     * @param fov FOV in degrees.
     * @param aspect Aspect ratio.
     * @param z_near Position of near plane.
     * @param z_far Position of far plane.
     */
    Camera(float fov, float aspect, float z_near, float z_far);

    /**
     * @brief Creates a Camera object.
     * @param info Information needed to create a projection matrix.
     */
    explicit Camera(const PersProjInfo& info);

    /**
     * Projection matrix is unchanged until the change of parameters in PersProjInfo.
     * In case of change in those parameters, it is necessary to call createProjection before this method.
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

    /**
     * Creates a projection matrix and stores it.
     * Should be called after a change to PersProjInfo parameters.
     */
    void createProjection();

    /**
     * Creates a view matrix and stores it.
     * Should be called after a change to orientation or getPosition.
     */
    void createView();

    /**
     * @brief Returns a getPosition of the camera in world space.
     * @return Reference to a getPosition in worlds space.
     */
    const glm::vec3& getPosition();

    /**
     * @brief Creates a normalized vector with the getDirection of the camera.
     * @return Normalized vector with the getDirection of the camera.
     */
    glm::vec3 getDirection();

    /**
     * Sets the getPosition of the camera in world space.
     * @param position Vector with getPosition in world space.
     */
    void setPosition(glm::vec3 position);

    /**
     * Sets the direction of the camera from a normalized getDirection vector.
     * @param direction Normalized getDirection vector.
     */
    void setDirection(glm::vec3 direction);

protected:
    glm::vec3 m_position;
    glm::quat m_orientation;

    [[nodiscard]] glm::vec3 forward() const {return Axis::NEG_Z * m_orientation;}
    [[nodiscard]] glm::vec3 back()    const {return Axis::POS_Z * m_orientation;}
    [[nodiscard]] glm::vec3 left()    const {return Axis::NEG_X * m_orientation;}
    [[nodiscard]] glm::vec3 right()   const {return Axis::POS_X * m_orientation;}
    [[nodiscard]] glm::vec3 down()    const {return Axis::NEG_Y * m_orientation;}
    [[nodiscard]] glm::vec3 up()      const {return Axis::POS_Y * m_orientation;}

private:
    inline glm::mat4x4 rotationMatrix(){
        return glm::toMat4(m_orientation);
    }

    inline glm::mat4x4 translationMatrix(){
        return glm::translate(glm::identity<glm::mat4x4>(), -m_position);
    }

    PersProjInfo m_persProjInfo;

    glm::mat4 m_viewMatrix = glm::identity<glm::mat4x4>();
    glm::mat4 m_projectionMatrix = glm::identity<glm::mat4x4>();
};

#endif //TECTONIC_CAMERA_H
