#include "camera/GameCamera.h"

void GameCamera::handleKeyEvent(u_short key) {
    switch(key){
        case GLFW_KEY_W:
            setPosition(m_position + m_speed * forward());
            break;
        case GLFW_KEY_S:
            setPosition(m_position + m_speed * back());
            sig_position.emit(m_position);
            break;
        case GLFW_KEY_A:
            setPosition(m_position + m_speed * left());
            break;
        case GLFW_KEY_D:
            setPosition(m_position + m_speed * right());
            break;
        case GLFW_KEY_SPACE:
            setPosition(m_position + glm::vec3(0.0,1.0f,0.0f) * m_speed);
            break;
        case GLFW_KEY_C:
            setPosition(m_position - glm::vec3(0.0,1.0f,0.0f) * m_speed);
            break;
        default:
            break;
    }
}

void GameCamera::handleMouseEvent(double x_in, double y_in) {
    if(m_cursorEnabled)
        return;

    auto x = static_cast<float>(x_in);
    auto y = static_cast<float>(y_in);

    if(m_firstMouse){
        m_lastMousePos = {x, y};
        m_firstMouse = false;
    }

    m_orientation *= glm::angleAxis((float)(x - m_lastMousePos.x) * m_sensitivity, Axis::POS_Y);
    m_orientation *= glm::angleAxis((float)(y - m_lastMousePos.y) * m_sensitivity, right());

    createVP();

    m_lastMousePos = {x, y};
}

void GameCamera::setSpeed(float speed) {
    m_speed = speed;
}
