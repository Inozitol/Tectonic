#pragma once

#include <connector/Signal.h>
#include <defs/CompilerDefs.h>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

/**
 * Represents a transformation.
 * Can be used to represent a local_position in arbitrary space.
 * Generates a transformation matrix, as well as inverse transformation matrix.
 */
struct Transformation {
    Transformation() = default;
    Transformation& operator=(const Transformation&);

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

    [[nodiscard]] always_inline const float& getScale() const { return scaleScalar; };
    [[nodiscard]] always_inline const glm::vec3& getRotation() const { return rotation; };
    [[nodiscard]] always_inline const glm::vec3& getTranslation() const { return translation; };

    /**
     * @brief Calculates and returns a reference to transformation matrix.
     * @return Reference to transformation matrix.
     */
    const glm::mat4& getMatrix() const;

    /**
     * @brief Calculates and returns a reference to inverse transformation matrix.
     * @return Reference to inverse transformation matrix.
     */
    [[nodiscard]] glm::mat4 getInverseMatrix() const;

    [[nodiscard]] glm::vec3 invertPosition(const glm::vec3& pos) const;
    [[nodiscard]] glm::vec3 invertDirection(const glm::vec3& dir) const;

    glm::mat4 scaleMatrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 translationMatrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    Signal<float> sig_scale;
    Signal<glm::vec3> sig_rotation;
    Signal<glm::vec3> sig_translation;
    Signal<> sig_changed;

private:
    /** Scalar scale value. */
    float scaleScalar = 1.0f;

    /** Rotation vector with axis rotations. */
    glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Translation vector with position. */
    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);

    mutable bool worldCurrent = false;
    mutable glm::mat4 worldMatrix{};
};
