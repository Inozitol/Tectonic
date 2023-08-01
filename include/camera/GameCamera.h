#ifndef TECTONIC_GAMECAMERA_H
#define TECTONIC_GAMECAMERA_H

#include "camera/Camera.h"

/**
 * Represents a controllable game camera. Usually only one per scene.
 * This camera can be controlled with keyboard and mouse by calling handleKeyEvent and handleMouseEvent.
 *
 * @brief Controllable camera.
 */
class GameCamera : public Camera {
public:
    GameCamera() = default;

    /**
     * @brief Handles an incoming key event and moves the camera accordingly.
     * @param key Key to handle.
     */
    void handleKeyEvent(u_short key);

    /**
     * Calculates a new angle for the camera to rotate to.
     * The class stores the previous mouse local_position and by calculating a difference with the current local_position
     * it gets a new angle of the camera.
     *
     * @brief Handles a mouse movement and orients the camera accordingly.
     * @param x Current mouse X local_position.
     * @param y Current mouse Y local_position.
     */
    void handleMouseEvent(double x, double y);
private:
    float m_speed = 0.05f;
    float m_sensitivity = 0.001f;

    bool m_firstMouse = true;
    glm::vec2 m_lastMousePos = glm::vec2(0.0f, 0.0f);
};


#endif //TECTONIC_GAMECAMERA_H
