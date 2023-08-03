#ifndef TECTONIC_LIGHTINGSHADER_H
#define TECTONIC_LIGHTINGSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "defs/ConfigDefs.h"
#include "Shader.h"
#include "Material.h"
#include "Transformation.h"
#include "camera/Camera.h"
#include "defs/ShaderDefines.h"

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
    DirectionalLight();

    glm::vec3 getDirection() { return m_lightView.getDirection(); };
    void setDirection(const glm::vec3& direction) { m_lightView.setDirection(direction); }

    void createView() { m_lightView.createView(); }
    void createProjection() { m_lightView.createProjectionMatrix(); }

    void updateTightOrthoProjection(const Camera& gameCamera);

    /**
     * Calculates the light's direction into a local space of provided transformation, usually of some mesh.
     * Should be called before localDirection.
     *
     * @param transform Transformation from which the light's direction gets transformed into local space.
     */
    void calcLocalDirection(Transformation& transform);

    /**
     * Returns a reference to getDirection of the light in local space of transformation provided by calcLocalDirection.
     * @return Reference to a getDirection in local space of a given transformation.
     */
    [[nodiscard]] const glm::vec3& localDirection() const { return m_localDirection; }

    glm::mat4 getWVP(glm::mat4 model){
        return m_lightView.getWVP(model);
    }

    OrthoProjInfo shadowOrthoInfo = {SHADOW_OPROJ_LEFT,
                                    SHADOW_OPROJ_RIGHT,
                                  SHADOW_OPROJ_BOTTOM,
                                     SHADOW_OPROJ_TOP,
                                   SHADOW_OPROJ_NEAR,
                                    SHADOW_OPROJ_FAR};
private:
    glm::vec3 m_localDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    Camera m_lightView = Camera();
};

/**
 * Represents a point light.
 * Point light has a position within a scene and emits light based off of its attenuation.
 */
class PointLight: public BaseLight {
public:
    struct {
        float constant = 1.0f;
        float linear = 0.0f;
        float exp = 0.0f;
    } attenuation;

    PointLight();

    /**
     * @brief Sets the position in world space.
     * @param position Vector with position in world space.
     */
    void setWorldPosition(glm::vec3 position) { return m_lightView.setPosition(position); }

    /**
     * @brief Returns the position in world space.
     * @return Position in world space.
     */
    [[nodiscard]] const glm::vec3& getWorldPosition() const { return m_lightView.getPosition(); }

    void createView() { m_lightView.createView(); }
    void createProjection() { m_lightView.createProjectionMatrix(); }

    /**
     * Calculates the light's position into a local space of provided transformation, usually of some mesh.
     * Should be called before getLocalPosition.
     *
     * @param transform Transformation from which the light's position gets transformed into local space.
     */
    void calcLocalPosition(Transformation& transform);

    /**
     * Returns a reference to position of the light in local space of transformation provided by calcLocalPosition.
     * @return Reference to a position in local space of a given transformation.
     */
    [[nodiscard]] const glm::vec3& getLocalPosition() const { return m_localPos; }

    void setDirection(glm::vec3 direction, glm::vec3 up = Axis::POS_Y) { m_lightView.setDirection(direction, up); };
    glm::vec3 getDirection() { return m_lightView.getDirection(); };

    glm::mat4 getWVP(glm::mat4 model){
        return m_lightView.getWVP(model);
    }

    const PerspProjInfo shadowPersInfo = {SHADOW_POINT_PPROJ_FOV,
                                          (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
                                          SHADOW_POINT_PPROJ_NEAR,
                                          SHADOW_POINT_PPROJ_FAR};
protected:
    glm::vec3 m_localPos = glm::vec3(0.0f, 0.0f, 0.0f);
    Camera m_lightView = Camera();
};

/**
 * Represents a spot light.
 * Spot lights acts like PointLight. It has attenuation and a position in world space.
 * It also defines a direction and an angle of effected light cone.
 */
class SpotLight : public PointLight {
public:

    SpotLight();

    void calcLocalDirectionPosition(Transformation& transform);
    [[nodiscard]] const glm::vec3& localDirection() const { return m_localDirection; }

    float angle = 0.0f;

    const PerspProjInfo shadowPersInfo = {SHADOW_SPOT_PPROJ_FOV,
                                          (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
                                          SHADOW_SPOT_PPROJ_NEAR,
                                          SHADOW_SPOT_PPROJ_FAR};
private:
    glm::vec3 m_localDirection = glm::vec3(0.0f, 0.0f, 0.0f);
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
    void setWorld(const glm::mat4& world) const;
    void setDiffuseTextureUnit(GLint tex_unit) const;
    void setSpecularTextureUnit(GLint tex_unit) const;
    void setShadowMapTextureUnit(GLint tex_unit) const;
    void setShadowCubeMapTextureUnit(GLint tex_unit) const;
    void setDirectionalLight(const DirectionalLight &light) const;
    void setLocalCameraPos(const glm::vec3& pos) const;
    void setMaterial(const Material& material) const;
    void setBoneTransform(uint32_t boneId, glm::mat4 transform) const;
    void setPointLights(GLint num_lights, const std::shared_ptr<PointLight> *light) const;
    void setSpotLights(GLint num_lights, const std::shared_ptr<SpotLight> *light) const;

private:
    // Position of g_gameCamera in local space
    GLint loc_local_camera_pos = -1;

    // Texture samplers for diffusion and specular lighting
    struct{
        GLint diffuse = -1;
        GLint specular = -1;
        GLint shadow_map = -1;
        GLint shadow_cube_map = -1;
    } loc_sampler;

    // WVP matrix
    GLint loc_wvp = -1;

    // Light WVP matrix
    GLint loc_light_wvp = -1;

    // World matrix
    GLint loc_world = -1;

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
        GLint local_position = -1;
        GLint world_position = -1;
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

    GLint loc_bone[MAX_BONES];
};


#endif //TECTONIC_LIGHTINGSHADER_H
