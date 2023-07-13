#ifndef TECTONIC_GAMECAMERA_H
#define TECTONIC_GAMECAMERA_H

#include "Camera.h"

/**
 * Represents a controllable game camera. Usually only one per scene.
 * This camera can be controlled with keyboard and mouse by calling handleKeyEvent and handleMouseEvent.
 *
 * @brief Controllable camera.
 */
class GameCamera : public Camera {
public:
    /**
     * @brief Creates a controllable GameCamera object.
     * @param fov FOV in degrees.
     * @param aspect Aspect ratio.
     * @param z_near Position of near plane.
     * @param z_far Position of far plane.
     */
    GameCamera(float fov, float aspect, float z_near, float z_far);

    /**
     * @brief Creates a controllable GameCamera object.
     * @param info Information needed to create a projection matrix.
     */
    GameCamera(const PersProjInfo& info);

    /**
     * @brief Handles an incoming key event and moves the camera accordingly.
     * @param key Key to handle.
     */
    void handleKeyEvent(u_short key);

    /**
     * Calculates a new angle for the camera to rotate to.
     * The class stores the previous mouse position and by calculating a difference with the current position
     * it gets a new angle of the camera.
     *
     * @brief Handles a mouse movement and orients the camera accordingly.
     * @param x Current mouse X position.
     * @param y Current mouse Y position.
     */
    void handleMouseEvent(double x, double y);
private:
    float m_speed = 0.05f;
    float m_sensitivity = 0.001f;

    bool m_firstMouse = true;
    glm::vec2 m_lastMousePos = glm::vec2(0.0f, 0.0f);
};


#endif //TECTONIC_GAMECAMERA_H
