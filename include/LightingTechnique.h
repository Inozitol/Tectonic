#ifndef TECTONIC_LIGHTINGTECHNIQUE_H
#define TECTONIC_LIGHTINGTECHNIQUE_H

#include <glm/ext/matrix_float4x4.hpp>
#include "Technique.h"
#include "Material.h"

class BaseLight{
public:
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ambient_intensity = 0.0f;
};

class DirectionalLight: public BaseLight{
public:
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
    float diffuse_intensity = 0.0f;

    void calc_local_direction(const glm::mat4x4& world);

    [[nodiscard]] const glm::vec3& local_direction() const { return _local_direction; }
private:
    glm::vec3 _local_direction = glm::vec3(0.0f, 0.0f, 0.0f);
};

class LightingTechnique : public Technique{
public:
    LightingTechnique();
    void init() override;

    void set_world(const glm::mat4x4& world) const;
    void set_view(const glm::mat4x4& view) const;
    void set_projection(const glm::mat4x4& projection) const;
    void set_diffuse_texture_unit(GLint tex_unit) const;
    void set_specular_texture_unit(GLint tex_unit) const;
    void set_directional_light(const DirectionalLight &light) const;
    void set_local_camera_pos(const glm::vec3& pos) const;
    void set_material(const Material& material) const;

private:

    GLint loc_local_camera_pos;

    struct{
        GLint diffuse;
        GLint specular;
    } loc_sampler;

    struct{
        GLint world;
        GLint view;
        GLint projection;
    } loc_wvp;

    struct{
        GLint ambient_color;
        GLint diffuse_color;
        GLint specular_color;
    } loc_material;

    struct{
        GLint color;
        GLint ambient_intensity;
        GLint direction;
        GLint diffuse_intensity;
    } loc_dir_light;
};


#endif //TECTONIC_LIGHTINGTECHNIQUE_H
