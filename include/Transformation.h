#ifndef TECTONIC_TRANSFORMATION_H
#define TECTONIC_TRANSFORMATION_H

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

class Transformation {
public:
    Transformation() = default;

    void set_scale(float scale);
    void set_rotation(float x, float y, float z);
    void set_translation(float x, float y, float z);

    void scale(float scale);
    void rotate(float x, float y, float z);
    void translate(float x, float y, float z);

    const glm::mat4x4& get_matrix();
    const glm::mat4x4& get_inverse_matrix();

private:
    glm::mat4x4 _scale_matrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4x4 _rotation_matrix = glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4x4 _translation_matrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4x4 _inv_scale_matrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4x4 _inv_rotation_matrix = glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4x4 _inv_translation_matrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4x4 _world_matrix{};
    glm::mat4x4 _inv_world_matrix{};
};


#endif //TECTONIC_TRANSFORMATION_H
