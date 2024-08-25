#ifndef TECTONIC_GAMECAMERA_H
#define TECTONIC_GAMECAMERA_H

#include <set>

#include "camera/Camera.h"
#include "Keyboard.h"
#include "connector/Slot.h"

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
     * @param buttonInfo Info about a keyboard button event.
     */
    void handleKeyboardEvent(const keyboardButtonInfo& buttonInfo);

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
     * Calculate new world space position from direction angles and direction vectors.
     * Should be called every frame.
     *
     * Delta time is pulled from TecCache.
     *
     * @brief Updates the world space of the camera.
     */
    void updatePosition();
    
    /**
     * Handles incoming keyboard presses.
     */
    Slot<keyboardButtonInfo> slt_keyEvent{[this](keyboardButtonInfo buttonInfo){
        handleKeyboardEvent(buttonInfo);
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
    /// Contains the set of all currently holding buttons.
    std::set<int32_t> m_holdingButtons;

    /// Sum of holding key destination vectors, calculated on every button press/release event.
    glm::vec3 m_destinationSumVec = {0.0f, 0.0f, 0.0f};

    /// Unit vector pointing to the current destination, calculated on every button press/release event.
    glm::vec3 m_destinationNormVec = {0.0f, 0.0f, 0.0f};

    float m_speed = 1.0f;
    float m_sensitivity = 0.001f;

    bool m_firstMouse = true;
    glm::vec2 m_lastMousePos = glm::vec2(0.0f, 0.0f);

    bool m_cursorEnabled = false;
};


#endif //TECTONIC_GAMECAMERA_H
