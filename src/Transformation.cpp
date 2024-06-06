#include "Transformation.h"

void Transformation::setScale(float scale) {
    m_scaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(scale, scale, scale));
    m_scale = scale;

    worldCurrent = false;
}

void Transformation::setRotation(float x, float y, float z) {
    glm::mat4x4 rx = glm::rotate(glm::mat4x4(1.0f), glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4x4 ry = glm::rotate(glm::mat4x4(1.0f), glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 rz = glm::rotate(glm::mat4x4(1.0f), glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));

    m_rotationMatrix = rx * ry * rz;
    m_rotation = {x,y,z};

    worldCurrent = false;
}

void Transformation::setTranslation(float x, float y, float z) {
    m_translationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(x, y, z));
    m_translation = {x,y,z};

    worldCurrent = false;
}

void Transformation::scale(float scale) {
    m_scaleMatrix = glm::scale(m_scaleMatrix, glm::vec3(scale, scale, scale));
    m_scale *= scale;

    worldCurrent = false;
}

void Transformation::rotate(float x, float y, float z) {
    m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));
    m_rotation += glm::vec3(x,y,z);

    worldCurrent = false;
}

void Transformation::translate(float x, float y, float z) {
    m_translationMatrix = glm::translate(m_translationMatrix, glm::vec3(x, y, z));
    m_translation += glm::vec3(x,y,z);

    worldCurrent = false;
}

const glm::mat4& Transformation::getMatrix() const{
    if(!worldCurrent) {
        m_worldMatrix = m_translationMatrix * m_scaleMatrix * m_rotationMatrix;
        worldCurrent = true;
    }

    return m_worldMatrix;
}

glm::mat4 Transformation::getInverseMatrix() const {
    if(!worldCurrent) {
        m_worldMatrix = m_translationMatrix * m_scaleMatrix * m_rotationMatrix;
        worldCurrent = true;
    }
    return glm::inverse(m_worldMatrix);
}

glm::vec3 Transformation::invertPosition(const glm::vec3& pos) const {
    glm::vec4 inv_pos = getInverseMatrix() * glm::vec4(pos, 1.0f);
    return inv_pos;
}

glm::vec3 Transformation::invertDirection(const glm::vec3& dir) const {
    glm::mat3 world3(getMatrix());
    world3 = glm::transpose(world3);
    return glm::normalize(world3 * dir);
}

float Transformation::getScale() const {
    return m_scale;
}

glm::vec3 Transformation::getRotation() const {
    return m_rotation;
}

glm::vec3 Transformation::getTranslation() const {
    return m_translation;
}
