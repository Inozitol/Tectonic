#include "engine/camera/GameCamera.h"
#include "engine/GlobalMemory.h"

void GameCamera::handleKeyboardEvent(const keyboardButtonInfo& buttonInfo) {
    if(buttonInfo.action == GLFW_PRESS) {
        m_holdingButtons.emplace(buttonInfo.key);
    }else if(buttonInfo.action == GLFW_RELEASE) {
        m_holdingButtons.erase(buttonInfo.key);
    }
    destinationSumVec = {0.0f, 0.0f, 0.0f};
    if(m_holdingButtons.empty()) {
        destinationNormVec = {0.0f, 0.0f, 0.0f};
        return;
    }
    for(const auto& key : m_holdingButtons) {
        switch(key){
            case GLFW_KEY_W:
                destinationSumVec += Axis::NEG_Z;
            break;
            case GLFW_KEY_S:
                destinationSumVec += Axis::POS_Z;
            break;
            case GLFW_KEY_A:
                destinationSumVec += Axis::NEG_X;
            break;
            case GLFW_KEY_D:
                destinationSumVec += Axis::POS_X;
            break;
            case GLFW_KEY_SPACE:
                destinationSumVec += Axis::POS_Y;
            break;
            case GLFW_KEY_C:
                destinationSumVec += Axis::NEG_Y;
            break;
            default:
                break;
        }
    }
    if(destinationSumVec == glm::vec3(0.0f, 0.0f, 0.0f)) {
        destinationNormVec = {0.0f, 0.0f, 0.0f};
    }else {
        destinationNormVec = glm::normalize(destinationSumVec);
    }
}

void GameCamera::updatePosition() {
    glm::vec3 directionRotated = destinationNormVec * orientation;
    setPosition(position + directionRotated * TecCorePtr->deltaTime * speed);
}

void GameCamera::handleMouseEvent(double x_in, double y_in) {
    if(cursorEnabled)
        return;

    auto x = static_cast<float>(x_in);
    auto y = static_cast<float>(y_in);

    if(firstMouse){
        lastMousePos = {x, y};
        firstMouse = false;
    }

    orientation *= glm::angleAxis((float)(x - lastMousePos.x) * sensitivity, Axis::POS_Y);
    orientation *= glm::angleAxis((float)(y - lastMousePos.y) * sensitivity, right());

    createVP();

    lastMousePos = {x, y};
}

void GameCamera::setSpeed(float s) {
    speed = s;
}
