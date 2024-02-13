#ifndef TECTONIC_GAMECAMERA_H
#define TECTONIC_GAMECAMERA_H

#include "camera/Camera.h"
#include "Keyboard.h"
#include "meta/Signal.h"
#include "meta/Slot.h"

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

    /**
     * @brief Sets a new camera speed.
     * @param speed New camera speed.
     */
    void setSpeed(float speed);

    /**
     * Handles incoming keyboard presses.
     */
    Slot<int32_t> slt_keyEvent{[this](int32_t key){
        handleKeyEvent(key);
    }};

    /**
     * Handles new mouse positions.
     */
    Slot<double, double> slt_mouseMovement{[this](double x, double y){
        handleMouseEvent(x,y);
    }};

    /**
     * Enables or disables cursor for the camera.
     * It needs to set m_firstMouse to true so that camera wont snap between previous positions.
     */
    Slot<bool> slt_cursorEnabled{[this](bool isEnabled){
        m_cursorEnabled = isEnabled;
        m_firstMouse = true;
    }};

private:
    float m_speed = 0.05f;
    float m_sensitivity = 0.001f;

    bool m_firstMouse = true;
    glm::vec2 m_lastMousePos = glm::vec2(0.0f, 0.0f);

    bool m_cursorEnabled = false;
};


#endif //TECTONIC_GAMECAMERA_H
