#ifndef TECTONIC_TRANSFORMATION_H
#define TECTONIC_TRANSFORMATION_H

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

/**
 * Represents a transformation.
 * Can be used to represent a position in arbitrary space.
 * Generates a transformation matrix, as well as inverse transformation matrix.
 */
class Transformation {
public:
    Transformation() = default;

    /**
     * @brief Overwrites the current scale.
     * @param scale Factor of uniform scale.
     */
    void setScale(float scale);

    /**
     * @brief Overwrites the current rotation.
     * @param x Degrees of rotation by X axis.
     * @param y Degrees of rotation by Y axis.
     * @param z Degrees of rotation by Z axis.
     */
    void setRotation(float x, float y, float z);

    /**
     * @brief Overwrites the current translation.
     * @param x Position along the X axis.
     * @param y Position along the Y axis.
     * @param z Position along the Z axis.
     */
    void setTranslation(float x, float y, float z);

    /**
     * @brief Scales the transformation.
     * @param scale Factor of uniform transformation.
     */
    void scale(float scale);

    /**
     * @brief Rotates the transformation.
     * @param x Degrees of rotation by X axis.
     * @param y Degrees of rotation by Y axis.
     * @param z Degrees of rotation by Z axis.
     */
    void rotate(float x, float y, float z);

    /**
     * @brief Translates the transformation.
     * @param x Position along the X axis.
     * @param y Position along the Y axis.
     * @param z Position along the Z axis.
     */
    void translate(float x, float y, float z);

    /**
     * @brief Calculates and returns a reference to transformation matrix.
     * @return Reference to transformation matrix.
     */
    const glm::mat4x4& getMatrix();

    /**
     * @brief Calculates and returns a reference to inverse transformation matrix.
     * @return Reference to inverse transformation matrix.
     */
    const glm::mat4x4& getInverseMatrix();

    glm::vec3 invert(const glm::vec3& pos);

private:
    glm::mat4 m_scaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 m_rotationMatrix = glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 m_translationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 m_invScaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 m_invRotationMatrix = glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 m_invTranslationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 m_worldMatrix{};
    glm::mat4 m_invWorldMatrix{};
};

#endif //TECTONIC_TRANSFORMATION_H