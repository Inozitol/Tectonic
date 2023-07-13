#include <glm/gtx/string_cast.hpp>
#include "Camera.h"

Camera::Camera(float fov, float aspect, float z_near, float z_far): Camera(PersProjInfo{fov, aspect, z_near, z_far}){}

Camera::Camera(const PersProjInfo &info): m_persProjInfo(info),
                                          m_orientation(glm::quatLookAt(Axis::NEG_Z, Axis::POS_Y)),
                                          m_position(0.0f, 0.0f, 0.0f){
    // Initialize projection matrix
    createProjection();
}

const glm::mat4x4& Camera::getViewMatrix() const {
    return m_viewMatrix;
}

const glm::mat4x4& Camera::getProjectionMatrix() const {
    return m_projectionMatrix;
}

void Camera::createProjection() {
    m_projectionMatrix = glm::perspective(glm::radians(m_persProjInfo.fov),
                                          m_persProjInfo.ratio,
                                          m_persProjInfo.zNear,
                                          m_persProjInfo.zFar);
}

void Camera::createView() {
    m_viewMatrix = rotationMatrix() * translationMatrix();
}

const glm::vec3& Camera::getPosition() {
    return m_position;
}

glm::vec3 Camera::getDirection() {
    return glm::normalize(forward());
}

void Camera::setDirection(glm::vec3 direction) {
    m_orientation = glm::conjugate(glm::quatLookAt(direction, Axis::POS_Y));
}

void Camera::setPosition(glm::vec3 position) {
    m_position = position;
}

