#include <glm/gtx/string_cast.hpp>
#include <memory>
#include "camera/Camera.h"
#include "exceptions.h"

const glm::mat4x4& Camera::getViewMatrix() const {
    return m_viewMatrix;
}

const glm::mat4x4& Camera::getProjectionMatrix() const {
    return m_projectionMatrix;
}

void Camera::createProjectionMatrix() {
    if(isPerspective){
        if(m_perspProjInfo){
            m_projectionMatrix = glm::perspective(glm::radians(m_perspProjInfo->fov),
                                                  m_perspProjInfo->aspect,
                                                  m_perspProjInfo->zNear,
                                                  m_perspProjInfo->zFar);
        }else{
            throw cameraException("Camera doesn't contain perspective projection info.");
        }
    }else{
        if(m_orthoProjInfo){
            m_projectionMatrix = glm::ortho(m_orthoProjInfo->left,
                                            m_orthoProjInfo->right,
                                            m_orthoProjInfo->bottom,
                                            m_orthoProjInfo->top,
                                            m_orthoProjInfo->zNear,
                                            m_orthoProjInfo->zFar);
        }else{
            throw cameraException("Camera doesn't contain orthographic projection info.");
        }
    }
}

glm::mat4 Camera::getWVP(const glm::mat4& model) {
    return m_projectionMatrix * m_viewMatrix * model;
}

void Camera::createView() {
    m_viewMatrix = rotationMatrix() * translationMatrix();
}

const glm::vec3 & Camera::getPosition() const {
    return m_position;
}

glm::vec3 Camera::getDirection() const {
    return glm::normalize(forward());
}

void Camera::setDirection(glm::vec3 direction, glm::vec3 up) {
    m_orientation = glm::conjugate(glm::quatLookAt(direction, up));
}

void Camera::setPosition(glm::vec3 position) {
    m_position = position;
}

void Camera::setPerspectiveInfo(const PerspProjInfo &info) {
    m_perspProjInfo = std::make_unique<PerspProjInfo>(info);
}

void Camera::setOrthographicInfo(const OrthoProjInfo &info) {
    m_orthoProjInfo = std::make_unique<OrthoProjInfo>(info);
}

void Camera::switchPerspective() {
    isPerspective = true;
}

void Camera::switchOrthographic() {
    isPerspective = false;
}

void Camera::toggleProjection() {
    isPerspective = !isPerspective;
}

Camera::~Camera() {
    m_perspProjInfo.reset(nullptr);
    m_orthoProjInfo.reset(nullptr);
}

Camera::Camera(const Camera& camera)
    :m_perspProjInfo(new PerspProjInfo(*camera.m_perspProjInfo)),
     m_orthoProjInfo(new OrthoProjInfo(*camera.m_orthoProjInfo)){}

const PerspProjInfo &Camera::getPerspectiveInfo() const{
    return *m_perspProjInfo;
}

const OrthoProjInfo &Camera::getOrthographicInfo() const{
    return *m_orthoProjInfo;
}
