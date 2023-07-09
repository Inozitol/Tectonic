#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include "Camera.h"

Camera::Camera(float fov, float aspect, float z_near, float z_far):
        _fov(fov), _aspect(aspect), _z_far(z_far), _z_near(z_near),
        _orientation(glm::quatLookAt(Axis::POS_Z, Axis::POS_Y)), _position(0.0f,0.0f,0.0f){
    _projection_matrix = glm::perspective(glm::radians(_fov), _aspect, _z_near, _z_far);
}

const glm::mat4x4& Camera::view_matrix() {
    _view_matrix = rotation_matrix() * translation_matrix();
    return _view_matrix;
}

const glm::mat4x4& Camera::projection_matrix() {
    return _projection_matrix;
}

const glm::vec3& Camera::position() {
    return _position;
}

void Camera::keyboard_event(u_short key) {
    switch(key){
        case GLFW_KEY_W:
            _position += _speed * forward();
            break;
        case GLFW_KEY_S:
            _position += _speed * back();
            break;
        case GLFW_KEY_A:
            _position += _speed * left();
            break;
        case GLFW_KEY_D:
            _position += _speed * right();
            break;
        case GLFW_KEY_SPACE:
            _position.y += _speed;
            break;
        case GLFW_KEY_C:
            _position.y -= _speed;
            break;
        default:
            break;
    }
}

void Camera::mouse_event(double x_in, double y_in) {
    auto x = static_cast<float>(x_in);
    auto y = static_cast<float>(y_in);

    if(first_mouse){
        _last_mouse_pos = {x, y};
        first_mouse = false;
    }

    _orientation *= glm::angleAxis((float)(x - _last_mouse_pos.x) * _sensitivity, Axis::POS_Y);
    _orientation *= glm::angleAxis((float)(y - _last_mouse_pos.y) * _sensitivity, right());

    _last_mouse_pos = {x,y};
}
