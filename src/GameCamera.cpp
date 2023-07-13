#include "GameCamera.h"

GameCamera::GameCamera(float fov, float aspect, float z_near, float z_far) : Camera(fov, aspect, z_near, z_far){}
GameCamera::GameCamera(const PersProjInfo& info) : Camera(info){}

void GameCamera::handleKeyEvent(u_short key) {
    switch(key){
        case GLFW_KEY_W:
            m_position += m_speed * forward();
            break;
        case GLFW_KEY_S:
            m_position += m_speed * back();
            break;
        case GLFW_KEY_A:
            m_position += m_speed * left();
            break;
        case GLFW_KEY_D:
            m_position += m_speed * right();
            break;
        case GLFW_KEY_SPACE:
            m_position.y += m_speed;
            break;
        case GLFW_KEY_C:
            m_position.y -= m_speed;
            break;
        default:
            break;
    }
}

void GameCamera::handleMouseEvent(double x_in, double y_in) {
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
