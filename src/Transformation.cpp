#include "Transformation.h"

void Transformation::setScale(float scale) {
    m_scaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(scale, scale, scale));
    m_invScaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1 / scale, 1 / scale, 1 / scale));
}

void Transformation::setRotation(float x, float y, float z) {
    glm::mat4x4 rx = glm::rotate(glm::mat4x4(1.0f), glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4x4 ry = glm::rotate(glm::mat4x4(1.0f), glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 rz = glm::rotate(glm::mat4x4(1.0f), glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4x4 irx = glm::rotate(glm::mat4x4(1.0f), glm::radians(-x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4x4 iry = glm::rotate(glm::mat4x4(1.0f), glm::radians(-y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 irz = glm::rotate(glm::mat4x4(1.0f), glm::radians(-z), glm::vec3(0.0f, 0.0f, 1.0f));

    m_rotationMatrix = rx * ry * rz;
    m_invRotationMatrix = irz * iry * irx;
}

void Transformation::setTranslation(float x, float y, float z) {
    m_translationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(x, y, z));
    m_invTranslationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(-x, -y, -z));
}

void Transformation::scale(float scale) {
    m_scaleMatrix = glm::scale(m_scaleMatrix, glm::vec3(scale, scale, scale));
    m_invScaleMatrix = glm::scale(m_scaleMatrix, glm::vec3(1 / scale, 1 / scale, 1 / scale));
}

void Transformation::rotate(float x, float y, float z) {
    m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    m_rotationMatrix = glm::rotate(m_rotationMatrix, glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));

    m_invRotationMatrix = glm::rotate(m_invRotationMatrix, glm::radians(-z), glm::vec3(0.0f, 0.0f, 1.0f));
    m_invRotationMatrix = glm::rotate(m_invRotationMatrix, glm::radians(-y), glm::vec3(0.0f, 1.0f, 0.0f));
    m_invRotationMatrix = glm::rotate(m_invRotationMatrix, glm::radians(-x), glm::vec3(1.0f, 0.0f, 0.0f));
}

void Transformation::translate(float x, float y, float z) {
    m_translationMatrix = glm::translate(m_translationMatrix, glm::vec3(x, y, z));
    m_invTranslationMatrix = glm::translate(m_invTranslationMatrix, glm::vec3(-x, -y, -z));
}

const glm::mat4x4& Transformation::getMatrix() {
    m_worldMatrix = m_translationMatrix * m_scaleMatrix * m_rotationMatrix;
    return m_worldMatrix;
}

const glm::mat4x4 &Transformation::getInverseMatrix() {
    m_invWorldMatrix = m_invRotationMatrix * m_invScaleMatrix * m_invTranslationMatrix;
    return m_invWorldMatrix;
}

glm::vec3 Transformation::invert(const glm::vec3& pos) {
    glm::vec4 inv_pos = getInverseMatrix() * glm::vec4(pos, 1.0f);
    return inv_pos;
}
