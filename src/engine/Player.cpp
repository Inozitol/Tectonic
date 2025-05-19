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
    playerItem.procedure = [this] { runImGui(); };
    ImGuiHandler::getMenu("GPU").items.emplace(playerItem.name, playerItem);
    ImGuiHandler::toggleMenu("GPU", "Player");
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

    if(controlsKeyGroup->heldKeys.empty() && state != State::FLYING) {
        if(state == State::WALKING) state = State::STANDING;
        return;
    }
    bool moved = false;
    for(const auto &key: controlsKeyGroup->heldKeys) {
        switch(key) {
            case GLFW_KEY_W:
                velocity.z += -walkSpeed;
                moved = true;
                break;
            case GLFW_KEY_S:
                velocity.z += +walkSpeed;
                moved = true;
                break;
            case GLFW_KEY_A:
                velocity.x += -walkSpeed;
                moved = true;
                break;
            case GLFW_KEY_D:
                velocity.x += +walkSpeed;
                moved = true;
                break;
            default:
                break;
        }
    }
    if(state == State::STANDING && moved) { state = State::WALKING; }
    if((state == State::WALKING || state == State::STANDING || state == State::SYNC_FALLING) && controlsKeyGroup->heldKeys.contains(GLFW_KEY_SPACE)) {
        state = State::JUMPING;
        jumpTime = 0.0f;
        fallTime = 0.0f;
        justJumped = true;
    }
    if(state == State::FLYING) {
        velocity.y = 0.0f;
        const bool holdsSpace = controlsKeyGroup->heldKeys.contains(GLFW_KEY_SPACE);
        const bool holdsC = controlsKeyGroup->heldKeys.contains(GLFW_KEY_C);
        if(holdsSpace) {
            velocity.y = walkSpeed;
        } else {
            velocity.y = holdsC ? velocity.y : 0.0f;
        }

        if(holdsC) {
            velocity.y = -walkSpeed;
        } else {
            velocity.y = holdsSpace ? velocity.y : 0.0f;
        }
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
    boundingBox.rotation = -glm::radians(yaw);//glm::rotateY({0.0f,0.0f,-1.0f},yaw);
}

void Player::updatePosition() {
    if(state == State::FLYING) { boundingBox.center += velocity * camera.orientation; } else { boundingBox.center += velocity * MathUtils::quatGetYRot(camera.orientation); }

    glm::vec3 newPosition = boundingBox.center;
    newPosition.y += 1.0f;

    camera.setPosition(newPosition);
    //boundingBox.center.y -= 1.0f;
}

std::tuple<float, float, float> Player::getBBHeight() const {
    BB::SideCorners_t boxBottom = boundingBox.getSideCorners(BB::Side::YNEG);

    // Get average height of the bounding box
    float hBoxAvg = 0.0f;
    float hTerrainAvg = 0.0f;
    for(uint8_t pIndex = 0; pIndex < BB::BOX_SIDE_CORNERS; ++pIndex) {
        const auto &p = boxBottom[pIndex];
        hBoxAvg += p.y;
        hTerrainAvg += TecCorePtr->world->terrain->patchManager.getHeightAt(p.x, p.z);
    }
    hTerrainAvg /= 4.0f;
    hBoxAvg /= 4.0f;
    // Measure distance from ground
    return {hTerrainAvg, hBoxAvg, hBoxAvg - hTerrainAvg};
}

void Player::updatePhysics() {
    auto [hTerrainAvg, hBoxAvg, hDist] = getBBHeight();
repeat_state:
    switch(state) {
        case State::UNKNOWN:
            if(hDist < heightDelta) {
                if(velocity.x != 0.0f || velocity.z != 0.0f) {
                    state = State::WALKING;
                    goto repeat_state;
                }
                state = State::STANDING;
            } else {
                state = State::SYNC_FALLING;
                goto repeat_state;
            }
            break;
        // When standing, stay in place
        case State::STANDING:
            boundingBox.setSideAxisScalar(BB::Side::YNEG, hTerrainAvg);
            break;
        case State::WALKING:
            if(hDist < heightDelta) { boundingBox.setSideAxisScalar(BB::Side::YNEG, hTerrainAvg); } else {
                state = State::SYNC_FALLING;
                goto repeat_state;
            }
            break;
        case State::SYNC_FALLING:
            if(hDist > syncFallDelta) {
                state = State::LONG_FALLING;
                goto repeat_state;
            }
            if(hDist < heightDelta) {
                fallTime = 0.0f;
                velocity.y = 0.0f;
                if(velocity.x != 0.0f || velocity.z != 0.0f) state = State::WALKING;
                else state = State::STANDING;
                goto repeat_state;
            }
        // Still falling
            fallTime += TecCorePtr->deltaTime;
            velocity.y = Physics::velocityFall<Physics::Object::HUMAN>(mass, fallTime);
            std::println("[Sync Fall] Time: {}, Vel: {} ", fallTime, velocity.y);
            break;
        case State::LONG_FALLING:
            if(hDist < heightDelta) {
                // Hit the ground
                fallImpactCallback();
                fallTime = 0.0f;
                velocity.y = 0.0f;
                if(velocity.x != 0.0f || velocity.z != 0.0f) state = State::WALKING;
                else state = State::STANDING;
                goto repeat_state;
            }
        // Still falling
            fallTime += TecCorePtr->deltaTime;
            velocity.y = Physics::velocityFall<Physics::Object::HUMAN>(mass, fallTime);
            std::println("[Long Fall] Time: {}, Vel: {} ", fallTime, velocity.y);
            break;
        case State::JUMPING:
            if(!justJumped && hDist < heightDelta) {
                // Hit the ground
                jumpImpactCallback();
                jumpTime = 0.0f;
                velocity.y = 0.0f;
                if(velocity.x != 0.0f || velocity.z != 0.0f) state = State::WALKING;
                else state = State::STANDING;
                goto repeat_state;
            }
        // Still jumping
            jumpTime += TecCorePtr->deltaTime;
            velocity.y = Physics::velocityJump<Physics::Object::HUMAN>(mass, jumpForce, jumpTime * jumpSpeed);
            justJumped = false;

            std::println("[Jump] Time: {}, Vel: {} ", jumpTime, velocity.y);
        case State::FLYING:
            break;
    }
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
    ImGuiTec::Text("State", Utils::enumVal(state));
}