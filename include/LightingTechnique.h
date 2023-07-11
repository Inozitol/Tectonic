#ifndef TECTONIC_LIGHTINGTECHNIQUE_H
#define TECTONIC_LIGHTINGTECHNIQUE_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Technique.h"
#include "Material.h"
#include "Transformation.h"

struct BaseLight{
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ambient_intensity = 0.0f;
    float diffuse_intensity = 0.0f;
};

class DirectionalLight: public BaseLight {
public:
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);

    void calc_local_direction(Transformation& transform);
    [[nodiscard]] const glm::vec3& local_direction() const { return _local_direction; }
private:
    glm::vec3 _local_direction = glm::vec3(0.0f, 0.0f, 0.0f);
};

class PointLight: public BaseLight {
public:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    struct{
        float constant = 1.0f;
        float linear = 0.0f;
        float exp = 0.0f;
    } attenuation;

    void calc_local_position(Transformation& transform);
    [[nodiscard]] const glm::vec3& local_position() const { return _local_pos; }
private:
    glm::vec3 _local_pos = glm::vec3(0.0f, 0.0f, 0.0f);
};

class SpotLight : public PointLight {
public:
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
    float angle = 0.0f;

    void calc_local_direction_position(Transformation& transform);
    [[nodiscard]] const glm::vec3& local_direction() const { return _local_direction; }
private:
    glm::vec3 _local_direction = glm::vec3(0.0f, 0.0f, 0.0f);
};

class LightingTechnique : public Technique {
public:
    LightingTechnique() = default;
    void init() override;

    void set_world(const glm::mat4x4& world) const;
    void set_view(const glm::mat4x4& view) const;
    void set_projection(const glm::mat4x4& projection) const;
    void set_diffuse_texture_unit(GLint tex_unit) const;
    void set_specular_texture_unit(GLint tex_unit) const;
    void set_directional_light(const DirectionalLight &light) const;
    void set_local_camera_pos(const glm::vec3& pos) const;
    void set_material(const Material& material) const;
    void set_point_lights(GLint num_lights, const PointLight* light) const;
    void set_spot_lights(GLint num_lights, const SpotLight* light) const;

    static const uint32_t MAX_POINT_LIGHTS = 2;
    static const uint32_t MAX_SPOT_LIGHTS = 2;

private:
    // Position of camera in local space
    GLint loc_local_camera_pos = -1;

    // Texture samplers for diffusion and specular lighting
    struct{
        GLint diffuse = -1;
        GLint specular = -1;
    } loc_sampler;

    // WVP matrices
    struct{
        GLint world = -1;
        GLint view = -1;
        GLint projection = -1;
    } loc_wvp;

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


#endif //TECTONIC_LIGHTINGTECHNIQUE_H
