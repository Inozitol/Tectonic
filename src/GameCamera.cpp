#include "camera/GameCamera.h"

void GameCamera::handleKeyEvent(u_short key) {
    switch(key){
        case GLFW_KEY_W:
            m_position += m_speed * forward();
            sig_position.emit(m_position);
            break;
        case GLFW_KEY_S:
            m_position += m_speed * back();
            sig_position.emit(m_position);
            break;
        case GLFW_KEY_A:
            m_position += m_speed * left();
            sig_position.emit(m_position);
            break;
        case GLFW_KEY_D:
            m_position += m_speed * right();
            sig_position.emit(m_position);
            break;
        case GLFW_KEY_SPACE:
            m_position.y += m_speed;
            sig_position.emit(m_position);
            break;
        case GLFW_KEY_C:
            m_position.y -= m_speed;
            sig_position.emit(m_position);
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

    m_lastMousePos = {x, y};
}
