#include "Transformation.h"

void Transformation::set_scale(float scale) {
    _scale_matrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(scale, scale, scale));
    _inv_scale_matrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1/scale, 1/scale, 1/scale));
}

void Transformation::set_rotation(float x, float y, float z) {
    glm::mat4x4 rx = glm::rotate(glm::mat4x4(1.0f), glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4x4 ry = glm::rotate(glm::mat4x4(1.0f), glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 rz = glm::rotate(glm::mat4x4(1.0f), glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4x4 irx = glm::rotate(glm::mat4x4(1.0f), glm::radians(-x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4x4 iry = glm::rotate(glm::mat4x4(1.0f), glm::radians(-y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 irz = glm::rotate(glm::mat4x4(1.0f), glm::radians(-z), glm::vec3(0.0f, 0.0f, 1.0f));

    _rotation_matrix = rx * ry * rz;
    _inv_rotation_matrix = irz * iry * irx;
}

void Transformation::set_translation(float x, float y, float z) {
    _translation_matrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(x, y, z));
    _inv_translation_matrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(-x, -y, -z));
}

void Transformation::scale(float scale) {
    _scale_matrix = glm::scale(_scale_matrix, glm::vec3(scale, scale, scale));
    _inv_scale_matrix = glm::scale(_scale_matrix, glm::vec3(1/scale, 1/scale, 1/scale));
}

void Transformation::rotate(float x, float y, float z) {
    _rotation_matrix = glm::rotate(_rotation_matrix, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
    _rotation_matrix = glm::rotate(_rotation_matrix, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
    _rotation_matrix = glm::rotate(_rotation_matrix, glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));

    _inv_rotation_matrix = glm::rotate(_inv_rotation_matrix, glm::radians(-z), glm::vec3(0.0f, 0.0f, 1.0f));
    _inv_rotation_matrix = glm::rotate(_inv_rotation_matrix, glm::radians(-y), glm::vec3(0.0f, 1.0f, 0.0f));
    _inv_rotation_matrix = glm::rotate(_inv_rotation_matrix, glm::radians(-x), glm::vec3(1.0f, 0.0f, 0.0f));
}

void Transformation::translate(float x, float y, float z) {
    _translation_matrix = glm::translate(_translation_matrix, glm::vec3(x, y, z));
    _inv_translation_matrix = glm::translate(_inv_translation_matrix, glm::vec3(-x, -y, -z));
}

const glm::mat4x4& Transformation::get_matrix() {
    _world_matrix = _translation_matrix * _scale_matrix * _rotation_matrix;
    return _world_matrix;
}

glm::mat4x4 Transformation::copy_matrix() const{
    return _world_matrix;
}

const glm::mat4x4 &Transformation::get_inverse_matrix() {
    _inv_world_matrix = _inv_rotation_matrix * _inv_scale_matrix * _inv_translation_matrix;
    return _inv_world_matrix;
}

glm::mat4x4 Transformation::copy_inverse_matrix() const {
    return _inv_world_matrix;
}

glm::vec3 Transformation::invert(const glm::vec3& pos) {
    glm::vec4 inv_pos = get_inverse_matrix() * glm::vec4(pos, 1.0f);
    return inv_pos;
}

