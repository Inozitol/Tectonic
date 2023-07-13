#ifndef TECTONIC_LIGHTINGSHADER_H
#define TECTONIC_LIGHTINGSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "config_defs.h"
#include "Shader.h"
#include "Material.h"
#include "Transformation.h"
#include "Camera.h"
#include "ShaderDefines.h"

/**
 * Base representation of light factors.
 */
struct BaseLight{
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ambientIntensity = 0.0f;
    float diffuseIntensity = 0.0f;
};

/**
 * Represents a directional light.
 * Directional light doesn't have a position, only a direction.
 * Usually only one is needed throughout the scene.
 */
class DirectionalLight: public BaseLight {
public:
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);

    /**
     * Calculates the light's direction into a local space of provided transformation, usually of some mesh.
     * Should be called before localDirection.
     *
     * @param transform Transformation from which the light's direction gets transformed into local space.
     */
    void calcLocalDirection(Transformation& transform);

    /**
     * Returns a reference to direction of the light in local space of transformation provided by calcLocalDirection.
     * @return Reference to a direction in local space of a given transformation.
     */
    [[nodiscard]] const glm::vec3& localDirection() const { return m_localDirection; }
private:
    glm::vec3 m_localDirection = glm::vec3(0.0f, 0.0f, 0.0f);
};

/**
 * Represents a point light.
 * Point light has a position within a scene and emits light based off of its attenuation.
 */
class PointLight: public BaseLight {
public:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    struct {
        float constant = 1.0f;
        float linear = 0.0f;
        float exp = 0.0f;
    } attenuation;

    /**
     * Calculates the light's position into a local space of provided transformation, usually of some mesh.
     * Should be called before localPosition.
     *
     * @param transform Transformation from which the light's position gets transformed into local space.
     */
    void calcLocalPosition(Transformation& transform);

    /**
     * Returns a reference to position of the light in local space of transformation provided by calcLocalPosition.
     * @return Reference to a position in local space of a given transformation.
     */
    [[nodiscard]] const glm::vec3& localPosition() const { return m_localPos; }
private:
    glm::vec3 m_localPos = glm::vec3(0.0f, 0.0f, 0.0f);
};

/**
 * Represents a spot light.
 * Spot lights act like PointLight, in that it has an attenuation,
 * but it also defines a direction and angle of an effected light cone.
 */
class SpotLight : public PointLight {
public:
    glm::vec3 direction() { return m_lightView.getDirection(); };
    void setDirection(glm::vec3 direction) { m_lightView.setDirection(direction); };
    void createViewMatrix() { m_lightView.createView(); }
    [[nodiscard]] const glm::mat4& getViewMatrix() { return m_lightView.getViewMatrix(); }
    [[nodiscard]] const glm::mat4& getProjectionMatrix() { return m_lightView.getProjectionMatrix(); }
    float angle = 0.0f;

    void calcLocalDirectionPosition(Transformation& transform);
    [[nodiscard]] const glm::vec3& localDirection() const { return m_localDirection; }

    const PersProjInfo shadowPersInfo = {SHADOW_FOV, (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, SHADOW_NEAR, SHADOW_FAR};
private:
    glm::vec3 m_localDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    Camera m_lightView = Camera(shadowPersInfo);
};

/**
 * Class for controlling base rendering shader.
 * The shaders in this class render the main camera view point.
 */
class LightingShader : public Shader {
public:
    LightingShader() = default;
    void init() override;

    void setWVP(const glm::mat4x4& wvp) const;
    void setLightWVP(const glm::mat4x4& light_wvp) const;
    void setDiffuseTextureUnit(GLint tex_unit) const;
    void setSpecularTextureUnit(GLint tex_unit) const;
    void setShadowTextureUnit(GLint tex_unit) const;
    void setDirectionalLight(const DirectionalLight &light) const;
    void setLocalCameraPos(const glm::vec3& pos) const;
    void setMaterial(const Material& material) const;
    void setPointLights(GLint num_lights, const PointLight* light) const;
    void setSpotLights(GLint num_lights, const SpotLight* light) const;

private:
    // Position of game_camera in local space
    GLint loc_local_camera_pos = -1;

    // Texture samplers for diffusion and specular lighting
    struct{
        GLint diffuse = -1;
        GLint specular = -1;
        GLint shadow = -1;
    } loc_sampler;

    // WVP matrix
    GLint loc_wvp = -1;

    // Light WVP matrix
    GLint loc_light_wvp = -1;

    // Colors of a material
    struct{
        GLint ambient_color = -1;
        GLint diffuse_color = -1;
        GLint specular_color = -1;
    } loc_material;

    // Definition of a directional light
    struct{
        GLint color = -1;
        GLint ambient_intensity = -1;
        GLint direction = -1;
        GLint diffuse_intensity = -1;
    } loc_dir_light;

    // Definition of an array of point lights
    struct{
        GLint color = -1;
        GLint ambient_intensity = -1;
        GLint diffuse_intensity = -1;
        GLint position = -1;
        struct {
            GLint constant = -1;
            GLint linear = -1;
            GLint exp = -1;
        } atten;
    } loc_point_lights[MAX_POINT_LIGHTS];
    GLint loc_num_point_lights = -1;

    // Definition of an array of spot lights
    struct{
        GLint color = -1;
        GLint ambient_intensity = -1;
        GLint diffuse_intensity = -1;
        GLint position = -1;
        GLint direction = -1;
        GLint angle = -1;
        struct {
            GLint constant = -1;
            GLint linear = -1;
            GLint exp = -1;
        } atten;
    } loc_spot_lights[MAX_SPOT_LIGHTS];
    GLint loc_num_spot_light = -1;
};


#endif //TECTONIC_LIGHTINGSHADER_H
