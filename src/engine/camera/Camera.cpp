#include <glm/gtx/string_cast.hpp>
#include <memory>
#include "engine/camera/Camera.h"
#include "utils/exceptions.h"

void Camera::createProjectionMatrix() {
    if(isPerspective){
        if(perspProjInfo){
            projectionMatrix = glm::perspective(glm::radians(perspProjInfo->fov),
                                                  perspProjInfo->aspect,
                                                  perspProjInfo->zNear,
                                                  perspProjInfo->zFar);
        }else{
            throw cameraException("Camera doesn't contain perspective projection info.");
        }
    }else{
        if(orthoProjInfo){
            projectionMatrix = glm::ortho(orthoProjInfo->left,
                                            orthoProjInfo->right,
                                            orthoProjInfo->bottom,
                                            orthoProjInfo->top,
                                            orthoProjInfo->zNear,
                                            orthoProjInfo->zFar);
        }else{
            throw cameraException("Camera doesn't contain orthographic projection info.");
        }
    }
}

glm::mat4 Camera::genWVP(const glm::mat4& world) const {
    return m_VP * world;
}

glm::mat4 Camera::genVP() const{
    return m_VP;
}

void Camera::createView() {
    viewMatrix = rotationMatrix() * translationMatrix();
}

void Camera::createVP() {
    createView();
    m_VP = projectionMatrix * viewMatrix;

    sig_VPMatrix.emit(m_VP);
}

glm::mat4 Camera::getVPNoTranslate() const {
    return projectionMatrix * rotationMatrix();
}

glm::vec3 * Camera::getPosition() {
    return &position;
}

glm::vec3 Camera::getDirection() const {
    return glm::normalize(forward());
}

void Camera::setDirection(glm::vec3 direction, glm::vec3 up) {
    orientation = glm::conjugate(glm::quatLookAt(direction, up));
    sig_orientation.emit(orientation);
    createVP();
}

void Camera::setPosition(glm::vec3 p) {
    position = p;
    sig_position.emit(p);
    createVP();
}

void Camera::setPerspectiveInfo(const PerspProjInfo &info) {
    perspProjInfo = std::make_unique<PerspProjInfo>(info);
}

void Camera::setOrthographicInfo(const OrthoProjInfo &info) {
    orthoProjInfo = std::make_unique<OrthoProjInfo>(info);
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
    perspProjInfo.reset(nullptr);
    orthoProjInfo.reset(nullptr);
}

Camera::Camera(const Camera& camera)
    :perspProjInfo(new PerspProjInfo(*camera.perspProjInfo)),
     orthoProjInfo(new OrthoProjInfo(*camera.orthoProjInfo)){}

const PerspProjInfo &Camera::getPerspectiveInfo() const{
    return *perspProjInfo;
}

const OrthoProjInfo &Camera::getOrthographicInfo() const{
    return *orthoProjInfo;
}

