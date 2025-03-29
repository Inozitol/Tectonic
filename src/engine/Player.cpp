#include "Player.h"
#include "GlobalMemory.h"
#include "engine/scene/World.h"
#include "math/MathUtils.h"
#include "math/Physics.h"
#include "utils/FloatLimit.h"
#include <glm/gtx/rotate_vector.hpp>
#include <print>

Player::Player() {
    initCamera();
    initImGui();
    controlsKeyGroup = &TecCorePtr->keyboard.keyGroups.at("controls");
    camera.setPosition({0.0f, 50.0f, 20.0f});
    boundingBox.halfLengths = {0.2f, 0.5f, 0.1f};
    boundingBox.center = camera.position;
    boundingBox.center.y -= 1.0f;
    VktCorePtr->debugGeometry.SAABBs[0] = {&boundingBox};
}

void Player::initImGui() {
    ImGuiHandler::ImGuiMenuItem playerItem;
    playerItem.name = "Player";
    playerItem.procedure = [this]{runImGui();};
    ImGuiHandler::getMenu("GPU").items.emplace(playerItem.name, playerItem);
}

void Player::frameUpdate() {
    updatePhysics();
    updatePosition();
}

void Player::initCamera() {
    camera.setPerspectiveInfo({CAMERA_PPROJ_FOV,
                               WindowPtr->getRatio(),
                               CAMERA_PPROJ_NEAR,
                               CAMERA_PPROJ_FAR});
    camera.createProjectionMatrix();

    TecCorePtr->world->terrain->connectCamera(PlayerPtr->camera);
}

void Player::handleKeyboardEvent() {
    velocity.x = velocity.z = 0.0f;

    if(controlsKeyGroup->heldKeys.empty()) { return; }
    for(const auto &key: controlsKeyGroup->heldKeys) {
        switch(key) {
            case GLFW_KEY_W:
                velocity.z += -walkSpeed;
                break;
            case GLFW_KEY_S:
                velocity.z += +walkSpeed;
                break;
            case GLFW_KEY_A:
                velocity.x += -walkSpeed;
                break;
            case GLFW_KEY_D:
                velocity.x += +walkSpeed;
                break;
            /*
            case GLFW_KEY_C:
                destinationSumVec += Axis::NEG_Y;
            break;*/
            default:
                break;
        }
    }
    if(!inAir && controlsKeyGroup->heldKeys.contains(GLFW_KEY_SPACE)) {
        justJumped = true;
        inAir = true;
        jumpTime = 0.0f;
    }
}

void Player::handleMouseEvent(double x_in, double y_in) {
    if(cursorEnabled) return;

    const glm::vec2 currMouse = {static_cast<float>(x_in), static_cast<float>(y_in)};

    if(firstMouse) {
        lastMousePos = currMouse;
        firstMouse = false;
    }

    const glm::vec2 mouseDelta = currMouse - lastMousePos;
    yaw += mouseDelta.x * sensitivity;
    pitch += mouseDelta.y * sensitivity;

    if(pitch >= 89.0f) pitch = 89.0f;
    if(pitch <= -89.0f) pitch = -89.0f;
    camera.orientation = glm::angleAxis(glm::radians(yaw), Axis::POS_Y);
    camera.orientation *= glm::angleAxis(glm::radians(pitch), camera.right());

    camera.createVP();

    lastMousePos = currMouse;

    glm::vec3 direction = camera.forward();
    direction.y = 0.0f;
    direction = glm::normalize(direction);
    //boundingBox.rotation = glm::rotation({0.0f,0.0f,-1.0f},direction);

}

void Player::updatePosition() {
    boundingBox.center += velocity * MathUtils::quatGetYRot(camera.orientation);

    glm::vec3 newPosition = boundingBox.center;
    newPosition.y += 1.0f;

    camera.setPosition(newPosition);
    //boundingBox.center.y -= 1.0f;
}

void Player::updatePhysics() {
    BB::SideCorners_t boxBottom = boundingBox.getSideCorners(BB::Side::YNEG);
    float hMax = -std::numeric_limits<float>::infinity();
    uint8_t pMaxIndex = 0;
    for(uint8_t pIndex = 0; pIndex < BB::BOX_SIDE_CORNERS; ++pIndex) {
        const auto &p = boxBottom[pIndex];
        float pHeight = TecCorePtr->world->terrain->patchManager.getHeightAt(p.x, p.z);
        if(pHeight > hMax) {
            hMax = pHeight;
            pMaxIndex = pIndex;
        }
    }
    float hDelta = boxBottom[pMaxIndex].y - hMax;
    if(hDelta < 0.01) {
        if(justJumped) {
            justJumped = false;
            jumpTime += TecCorePtr->deltaTime;
            velocity.y = Physics::velocityJump<Physics::Object::HUMAN>(mass, jumpForce, jumpTime*jumpSpeed);
            return;
        }
        if(inAir) fallImpact();
        fallTime = 0.0f;
        jumpTime = 0.0f;
        velocity.y = 0.0f;
        inAir = false;
        boundingBox.setSideAxisScalar(BB::Side::YNEG, hMax);
        return;
    }
    if(inAir && jumpTime > 0.0f) {
        jumpTime += TecCorePtr->deltaTime;
        velocity.y = Physics::velocityJump<Physics::Object::HUMAN>(mass, jumpForce, jumpTime*jumpSpeed);
        return;
    }

    inAir = true;
    fallTime += TecCorePtr->deltaTime;
    velocity.y = Physics::velocityFall<Physics::Object::HUMAN>(mass, fallTime);
    std::println("[Fall] Time: {}, Vel: {} ", fallTime, velocity.y);
}

void Player::runImGui() {
    IMGUI_WINDOW(Player)

    ImGui::SeparatorText("Controls");
    ImGui::DragFloat("Sensitivity", &sensitivity, 0.001f, 0.0f);
    ImGuiTec::Text("Quat", camera.orientation);
    ImGuiTec::Text("Last mouse", lastMousePos);
    ImGuiTec::Text("Yaw", yaw);
    ImGuiTec::Text("Pitch", pitch);


    ImGui::SeparatorText("Physics");
    ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f);
    ImGui::DragFloat("Jump Force", &jumpForce, 0.1f, 0.0f);
    ImGui::DragFloat("Jump speed", &jumpSpeed, 0.1f, 0.0f);
    ImGui::DragFloat("Walk speed", &walkSpeed, 0.1f, 0.0f);
    ImGuiTec::Text("Velocity", velocity);
    ImGuiTec::Text("Jump time", jumpTime);
    ImGuiTec::Text("Fall time", fallTime);
    ImGuiTec::Text("In air", inAir);
    ImGuiTec::Text("Just jumped", justJumped);
}