#pragma once
#include "camera/GameCamera.h"
#include "geometry/BB.h"
#include "geometry/SAABB.h"

struct Player {
    Player();

    void initImGui();
    void initCamera();

    Camera camera;

    void frameUpdate();

    /**
     * This is called every time a control key is pressed or released.
     * The keys that are held are kept in Keyboard, so it's unnencessary to send details on which key was just pressed.
     * @brief Handles an incoming key event and moves the camera accordingly.
     */
    void handleKeyboardEvent();

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
     * Calculate new world space position from direction angles and direction vectors.
     * Should be called every frame.
     *
     * Delta time is pulled from TecCache.
     *
     * @brief Updates the world space of the camera.
     */
    void updatePosition();

    void updatePhysics();
    void fallImpact(){}

    void runImGui();

    float jumpForce = 2.9f;
    float jumpSpeed = 2.0f;
    bool inAir = false;
    bool justJumped = false;
    float mass = 75.0f; // kg
    float fallTime = 0.0f;
    float jumpTime = 0.0f;
    float walkSpeed = 0.1f;

    float sensitivity = 0.1;

    /// Contains the set of all currently holding buttons.
    Keyboard::KeyGroup const * controlsKeyGroup = nullptr;

    /// Sum of holding key destination vectors, calculated on every button press/release event.
    glm::vec3 velocity = {0.0f, 0.0f, 0.0f};


    bool firstMouse = true;
    bool cursorEnabled = false;
    glm::vec2 lastMousePos = glm::vec2(0.0f, 0.0f);
    float yaw = 0.0f;
    float pitch = 0.0f;

    SAABB boundingBox{.axis = BB::Axis::Y};

    /**
     * Handles incoming keyboard presses.
     */
    Slot<> slt_keyEvent{[this]() { handleKeyboardEvent(); }};

    /**
     * Handles new mouse positions.
     */
    Slot<double, double> slt_mouseMovement{[this](double x, double y) { handleMouseEvent(x, y); }};

    /**
     * Enables or disables cursor for the camera.
     * It needs to set m_firstMouse to true so that camera wont snap between previous positions.
     */
    Slot<bool> slt_cursorEnabled{[this](bool isEnabled) {
        cursorEnabled = isEnabled;
        firstMouse = true;
    }};

};