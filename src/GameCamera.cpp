#include "camera/GameCamera.h"

#include "engine/TecCache.h"

void GameCamera::handleKeyboardEvent(const keyboardButtonInfo& buttonInfo) {
    if(buttonInfo.action == GLFW_PRESS) {
        m_holdingButtons.emplace(buttonInfo.key);
    }else if(buttonInfo.action == GLFW_RELEASE) {
        m_holdingButtons.erase(buttonInfo.key);
    }
    m_destinationSumVec = {0.0f, 0.0f, 0.0f};
    if(m_holdingButtons.empty()) {
        m_destinationNormVec = {0.0f, 0.0f, 0.0f};
        return;
    }
    for(const auto& key : m_holdingButtons) {
        switch(key){
            case GLFW_KEY_W:
                m_destinationSumVec += Axis::NEG_Z;
            break;
            case GLFW_KEY_S:
                m_destinationSumVec += Axis::POS_Z;
            break;
            case GLFW_KEY_A:
                m_destinationSumVec += Axis::NEG_X;
            break;
            case GLFW_KEY_D:
                m_destinationSumVec += Axis::POS_X;
            break;
            case GLFW_KEY_SPACE:
                m_destinationSumVec += Axis::POS_Y;
            break;
            case GLFW_KEY_C:
                m_destinationSumVec += Axis::NEG_Y;
            break;
            default:
                break;
        }
    }
    if(m_destinationSumVec == glm::vec3(0.0f, 0.0f, 0.0f)) {
        m_destinationNormVec = {0.0f, 0.0f, 0.0f};
    }else {
        m_destinationNormVec = glm::normalize(m_destinationSumVec);
    }
}

void GameCamera::updatePosition() {
    glm::vec3 directionRotated = m_destinationNormVec * m_orientation;
    m_position += directionRotated * TecCache::deltaTime * m_speed;
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
