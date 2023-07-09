#ifndef TECTONIC_CAMERA_H
#define TECTONIC_CAMERA_H

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>

namespace Axis{
    static constexpr glm::vec3 POS_X(1.0f,0.0f,0.0f);
    static constexpr glm::vec3 POS_Y(0.0f,1.0f,0.0f);
    static constexpr glm::vec3 POS_Z(0.0f,0.0f,1.0f);
    static constexpr glm::vec3 NEG_X(-1.0f,0.0f,0.0f);
    static constexpr glm::vec3 NEG_Y(0.0f,-1.0f,0.0f);
    static constexpr glm::vec3 NEG_Z(0.0f,0.0f,-1.0f);
}

class Camera {
public:
    explicit Camera(float fov, float aspect, float z_near, float z_far);

    void keyboard_event(u_short key);
    void mouse_event(double x, double y);

    const glm::mat4x4& projection_matrix();
    const glm::mat4x4& view_matrix();

    const glm::vec3& position();

private:
    inline glm::mat4x4 rotation_matrix(){
        return glm::toMat4(_orientation);
    }

    inline glm::mat4x4 translation_matrix(){
        return glm::translate(glm::identity<glm::mat4x4>(), -_position);
    }

    [[nodiscard]] glm::vec3 forward() const {return Axis::NEG_Z * _orientation;}
    [[nodiscard]] glm::vec3 back()    const {return Axis::POS_Z * _orientation;}
    [[nodiscard]] glm::vec3 left()    const {return Axis::NEG_X * _orientation;}
    [[nodiscard]] glm::vec3 right()   const {return Axis::POS_X * _orientation;}
    [[nodiscard]] glm::vec3 down()    const {return Axis::NEG_Y * _orientation;}
    [[nodiscard]] glm::vec3 up()      const {return Axis::POS_Y * _orientation;}

    float _fov;
    float _aspect;
    float _z_far;
    float _z_near;

    glm::vec3 _position;
    glm::quat _orientation;

    float _speed = 0.05f;
    float _sensitivity = 0.001f;

    glm::mat4x4 _view_matrix{};
    glm::mat4x4 _projection_matrix{};

    bool first_mouse = true;
    glm::vec2 _last_mouse_pos{};
};

#endif //TECTONIC_CAMERA_H
