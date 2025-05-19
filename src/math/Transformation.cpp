#include "Transformation.h"

Transformation & Transformation::operator=(const Transformation & other) {
    if(this == &other) { return *this; }

    this->scaleMatrix = other.scaleMatrix;
    this->rotationMatrix = other.rotationMatrix;
    this->translationMatrix = other.translationMatrix;
    this->scaleScalar = other.scaleScalar;
    this->rotation = other.rotation;
    this->translation = other.translation;
    this->worldCurrent = other.worldCurrent;
    this->worldMatrix = other.worldMatrix;

    return *this;
}

void Transformation::setScale(const float scale) {
    scaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(scale, scale, scale));
    scaleScalar = scale;
    worldCurrent = false;

    sig_scale.emit(scaleScalar);
    sig_changed.emit();
}

void Transformation::setRotation(const float x, const float y, const float z) {
    const glm::mat4x4 rx = glm::rotate(glm::mat4x4(1.0f), glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4x4 ry = glm::rotate(glm::mat4x4(1.0f), glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4x4 rz = glm::rotate(glm::mat4x4(1.0f), glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));
    rotationMatrix = rx * ry * rz;
    rotation = {x, y, z};
    worldCurrent = false;

    sig_rotation.emit(rotation);
    sig_changed.emit();
}

void Transformation::setTranslation(const float x, const float y, const float z) {
    translationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(x, y, z));
    translation = {x, y, z};
    worldCurrent = false;

    sig_translation.emit(translation);
    sig_changed.emit();
}

void Transformation::scale(const float scale) {
    scaleMatrix = glm::scale(scaleMatrix, glm::vec3(scale, scale, scale));
    scaleScalar *= scale;
    worldCurrent = false;

    sig_scale.emit(scaleScalar);
    sig_changed.emit();
}

void Transformation::rotate(const float x, const float y, const float z) {
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));
    rotation += glm::vec3(x, y, z);
    worldCurrent = false;

    sig_rotation.emit(rotation);
    sig_changed.emit();
}

void Transformation::translate(const float x, const float y, const float z) {
    translationMatrix = glm::translate(translationMatrix, glm::vec3(x, y, z));
    translation += glm::vec3(x, y, z);
    worldCurrent = false;

    sig_translation.emit(translation);
    sig_changed.emit();
}

const glm::mat4 &Transformation::getMatrix() const {
    if(!worldCurrent) {
        worldMatrix = translationMatrix * scaleMatrix * rotationMatrix;
        worldCurrent = true;
    }

    return worldMatrix;
}

glm::mat4 Transformation::getInverseMatrix() const {
    if(!worldCurrent) {
        worldMatrix = translationMatrix * scaleMatrix * rotationMatrix;
        worldCurrent = true;
    }

    return glm::inverse(worldMatrix);
}

glm::vec3 Transformation::invertPosition(const glm::vec3 &pos) const {
    glm::vec4 inv_pos = getInverseMatrix() * glm::vec4(pos, 1.0f);
    return inv_pos;
}

glm::vec3 Transformation::invertDirection(const glm::vec3 &dir) const {
    glm::mat3 world3(getMatrix());
    world3 = glm::transpose(world3);
    return glm::normalize(world3 * dir);
}