#ifndef TECTONIC_LIGHTINGSHADER_H
#define TECTONIC_LIGHTINGSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "defs/ConfigDefs.h"
#include "model/Material.h"
#include "Transformation.h"
#include "camera/Camera.h"
#include "defs/ShaderDefines.h"
#include "model/ModelTypes.h"
#include "Shader.h"

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

    [[nodiscard]] glm::vec3 getDirection() const { return m_lightView.getDirection(); };
    void setDirection(const glm::vec3& direction) { m_lightView.setDirection(direction); }

    void createView() { m_lightView.createView(); }
    void createProjection() { m_lightView.createProjectionMatrix(); }

    void updateTightOrthoProjection(const Camera& gameCamera);

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
    void setPosition(glm::vec3 position) { return m_lightView.setPosition(position); }

    /**
     * @brief Returns the position in world space.
     * @return Position in world space.
     */
    [[nodiscard]] const glm::vec3& getPosition() const { return m_lightView.getPosition(); }

    void createView() { m_lightView.createView(); }
    void createProjection() { m_lightView.createProjectionMatrix(); }

    void setDirection(glm::vec3 direction, glm::vec3 up = Axis::POS_Y) { m_lightView.setDirection(direction, up); };
    [[nodiscard]] glm::vec3 getDirection() const { return m_lightView.getDirection(); };

    glm::mat4 getWVP(glm::mat4 model){
        return m_lightView.getWVP(model);
    }

    const PerspProjInfo shadowPersInfo = {SHADOW_POINT_PPROJ_FOV,
                                          (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
                                          SHADOW_POINT_PPROJ_NEAR,
                                          SHADOW_POINT_PPROJ_FAR};
protected:
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

    float angle = 0.0f;

    const PerspProjInfo shadowPersInfo = {SHADOW_SPOT_PPROJ_FOV,
                                          (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
                                          SHADOW_SPOT_PPROJ_NEAR,
                                          SHADOW_SPOT_PPROJ_FAR};
};

/**
 * Class for controlling base rendering shader.
 * The shaders in this class render the main camera view point.
 */
class LightingShader : public Shader {
public:
    LightingShader(): Shader(ShaderType::BASIC_SHADER | ShaderType::BONE_SHADER){}
    void init() override;

    void setWVP(const glm::mat4x4& wvp) const;
    void setLightWVP(const glm::mat4x4& light_wvp) const;
    void setWorld(const glm::mat4& world) const;
    void setDiffuseTextureUnit(GLint texUnit) const;
    void setSpecularTextureUnit(GLint texUnit) const;
    void setNormalTextureUnit(GLint texUnit) const;
    void setShadowMapTextureUnit(GLint texUnit) const;
    void setShadowCubeMapTextureUnit(GLint texUnit) const;
    void setDirectionalLight(const DirectionalLight &light) const;
    void setWorldCameraPos(const glm::vec3& pos) const;
    void setMaterial(const Material& material) const;
    void setBoneTransforms(const boneTransfoms_t& transforms) const;
    void setPointLights(GLint num_lights, const std::array<PointLight, MAX_POINT_LIGHTS>& light) const;
    void setSpotLights(GLint num_lights, const std::array<SpotLight, MAX_SPOT_LIGHTS>& light) const;
    void setColorMod(const glm::vec4& clr) const;

private:

    // Position of gameCamera in world space
    uint32_t loc_worldCameraPos = -1;

    // Texture samplers for diffusion and specular lighting
    struct {
        uint32_t diffuse = -1;
        uint32_t specular = -1;
        uint32_t shadow_map = -1;
        uint32_t normal = -1;
        uint32_t shadow_cube_map = -1;
    } loc_sampler;

    // WVP matrix
    uint32_t loc_WVP = -1;

    // Light WVP matrix
    uint32_t loc_lightWVP = -1;

    // World matrix
    uint32_t loc_world = -1;

    // Colors of a material
    struct {
        uint32_t ambient_color = -1;
        uint32_t diffuse_color = -1;
        uint32_t specular_color = -1;
        uint32_t shininess = -1;
    } loc_material;

    // Definition of a directional light
    struct {
        uint32_t color = -1;
        uint32_t ambient_intensity = -1;
        uint32_t diffuse_intensity = -1;
        uint32_t direction = -1;
    } loc_dirLight;

    // Definition of an array of point lights
    struct {
        uint32_t color = -1;
        uint32_t ambient_intensity = -1;
        uint32_t diffuse_intensity = -1;
        uint32_t position = -1;
        struct {
            uint32_t constant = -1;
            uint32_t linear = -1;
            uint32_t exp = -1;
        } atten;
    } loc_pointLights[MAX_POINT_LIGHTS];
    uint32_t loc_numPointLights = -1;

    // Definition of an array of spot lights
    struct {
        uint32_t color = -1;
        uint32_t ambient_intensity = -1;
        uint32_t diffuse_intensity = -1;
        uint32_t position = -1;
        uint32_t direction = -1;
        uint32_t angle = -1;
        struct {
            uint32_t constant = -1;
            uint32_t linear = -1;
            uint32_t exp = -1;
        } atten;
    } loc_spotLights[MAX_SPOT_LIGHTS];
    uint32_t loc_numSpotLights = -1;

    uint32_t loc_colorMod = -1;

    // Array of bones inside the scene
    uint32_t loc_boneMatrixArray{};
};


#endif //TECTONIC_LIGHTINGSHADER_H
