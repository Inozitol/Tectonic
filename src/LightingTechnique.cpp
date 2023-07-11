#include <cstring>
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include "LightingTechnique.h"

void DirectionalLight::calc_local_direction(Transformation& transform) {
    glm::mat3x3 world3(transform.get_matrix());
    glm::mat3x3 world2local = glm::transpose(world3);   // Assuming uniform scaling
    _local_direction = glm::normalize(world2local * direction);
}

void PointLight::calc_local_position(Transformation& transform) {
    _local_pos = transform.invert(position);
}

void SpotLight::calc_local_direction_position(Transformation &transform) {
    PointLight::calc_local_position(transform);
    _local_direction = transform.invert(direction);
}

void LightingTechnique::init() {

    Technique::init();

    add_shader(GL_VERTEX_SHADER, "shaders/lighting.vs");
    add_shader(GL_FRAGMENT_SHADER, "shaders/lighting.fs");
    finalize();

    loc_local_camera_pos            = uniform_location("local_camera_pos");

    loc_wvp.world                   = uniform_location("wvp.world");
    loc_wvp.view                    = uniform_location("wvp.view");
    loc_wvp.projection              = uniform_location("wvp.projection");

    loc_sampler.diffuse             = uniform_location("sampler.diffuse");
    loc_sampler.specular            = uniform_location("sampler.specular");

    loc_material.ambient_color      = uniform_location("material.ambient_color");
    loc_material.diffuse_color      = uniform_location("material.diffuse_color");
    loc_material.specular_color     = uniform_location("material.specular_color");

    loc_dir_light.color             = uniform_location("directional_light.base.color");
    loc_dir_light.ambient_intensity = uniform_location("directional_light.base.ambient_intensity");
    loc_dir_light.diffuse_intensity = uniform_location("directional_light.base.diffuse_intensity");
    loc_dir_light.direction         = uniform_location("directional_light.direction");

    loc_num_point_lights            = uniform_location("num_point_lights");
    loc_num_spot_light              = uniform_location("num_spot_lights");

    if(loc_wvp.world == 0xFFFFFFFF ||
       loc_wvp.view == 0xFFFFFFFF ||
       loc_wvp.projection == 0xFFFFFFFF ||
       loc_sampler.diffuse == 0xFFFFFFFF ||
       loc_sampler.specular == 0xFFFFFFFF ||
       loc_material.ambient_color == 0xFFFFFFFF ||
       loc_material.diffuse_color == 0xFFFFFFFF ||
       loc_material.specular_color == 0xFFFFFFFF ||
       loc_dir_light.color == 0xFFFFFFFF ||
       loc_dir_light.ambient_intensity == 0xFFFFFFFF ||
       loc_dir_light.diffuse_intensity == 0xFFFFFFFF ||
       loc_dir_light.direction == 0xFFFFFFFF ||
       loc_num_point_lights == 0xFFFFFFFF){
        throw technique_exception("Invalid uniform locations");
    }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_point_lights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "point_lights[%d].base.color", i);
        loc_point_lights[i].color = uniform_location(name);

        snprintf(name, sizeof(name), "point_lights[%d].base.ambient_intensity", i);
        loc_point_lights[i].ambient_intensity = uniform_location(name);

        snprintf(name, sizeof(name), "point_lights[%d].base.diffuse_intensity", i);
        loc_point_lights[i].diffuse_intensity = uniform_location(name);

        snprintf(name, sizeof(name), "point_lights[%d].local_pos", i);
        loc_point_lights[i].position = uniform_location(name);

        snprintf(name, sizeof(name), "point_lights[%d].atten.constant", i);
        loc_point_lights[i].atten.constant = uniform_location(name);

        snprintf(name, sizeof(name), "point_lights[%d].atten.linear", i);
        loc_point_lights[i].atten.linear = uniform_location(name);

        snprintf(name, sizeof(name), "point_lights[%d].atten.exp", i);
        loc_point_lights[i].atten.exp = uniform_location(name);

        if(loc_point_lights[i].color == 0xFFFFFFFF ||
           loc_point_lights[i].ambient_intensity == 0xFFFFFFFF ||
           loc_point_lights[i].diffuse_intensity == 0xFFFFFFFF ||
           loc_point_lights[i].position == 0xFFFFFFFF ||
           loc_point_lights[i].atten.constant == 0xFFFFFFFF ||
           loc_point_lights[i].atten.linear == 0xFFFFFFFF ||
           loc_point_lights[i].atten.exp == 0xFFFFFFFF){
            throw technique_exception("Invalid uniform locations");
            }
        }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_spot_lights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "spot_lights[%d].base.base.color", i);
        loc_spot_lights[i].color = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].base.base.ambient_intensity", i);
        loc_spot_lights[i].ambient_intensity = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].base.base.diffuse_intensity", i);
        loc_spot_lights[i].diffuse_intensity = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].base.local_pos", i);
        loc_spot_lights[i].position = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].base.atten.constant", i);
        loc_spot_lights[i].atten.constant = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].base.atten.linear", i);
        loc_spot_lights[i].atten.linear = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].base.atten.exp", i);
        loc_spot_lights[i].atten.exp = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].direction", i);
        loc_spot_lights[i].direction = uniform_location(name);

        snprintf(name, sizeof(name), "spot_lights[%d].angle", i);
        loc_spot_lights[i].angle = uniform_location(name);

        if(loc_spot_lights[i].color == 0xFFFFFFFF ||
           loc_spot_lights[i].ambient_intensity == 0xFFFFFFFF ||
           loc_spot_lights[i].diffuse_intensity == 0xFFFFFFFF ||
           loc_spot_lights[i].position == 0xFFFFFFFF ||
           loc_spot_lights[i].atten.constant == 0xFFFFFFFF ||
           loc_spot_lights[i].atten.linear == 0xFFFFFFFF ||
           loc_spot_lights[i].atten.exp == 0xFFFFFFFF ||
           loc_spot_lights[i].direction == 0xFFFFFFFF ||
           loc_spot_lights[i].angle == 0xFFFFFFFF){
            throw technique_exception("Invalid uniform locations");
        }
    }

}

void LightingTechnique::set_world(const glm::mat4x4 &world) const {
    glUniformMatrix4fv(loc_wvp.world, 1, GL_FALSE, glm::value_ptr(world));
}

void LightingTechnique::set_view(const glm::mat4x4 &view) const {
    glUniformMatrix4fv(loc_wvp.view, 1, GL_FALSE, glm::value_ptr(view));
}

void LightingTechnique::set_projection(const glm::mat4x4& projection) const {
    glUniformMatrix4fv(loc_wvp.projection, 1, GL_FALSE, glm::value_ptr(projection));
}

void LightingTechnique::set_diffuse_texture_unit(GLint tex_unit) const {
    glUniform1i(loc_sampler.diffuse, tex_unit);
}

void LightingTechnique::set_specular_texture_unit(GLint tex_unit) const{
    glUniform1i(loc_sampler.specular, tex_unit);
}
void LightingTechnique::set_directional_light(const DirectionalLight &light) const {
    glUniform3f(loc_dir_light.color, light.color.r, light.color.g, light.color.b);
    glUniform1f(loc_dir_light.diffuse_intensity, light.diffuse_intensity);
    const glm::vec3 local_direction = light.local_direction();
    glUniform3f(loc_dir_light.direction, local_direction.x, local_direction.y, local_direction.z);
    glUniform1f(loc_dir_light.ambient_intensity, light.ambient_intensity);
}

void LightingTechnique::set_local_camera_pos(const glm::vec3 &pos) const {
    glUniform3f(loc_local_camera_pos, pos.x, pos.y, pos.z);
}

void LightingTechnique::set_material(const Material &material) const {
    glUniform3f(loc_material.diffuse_color, material.diffuse_color.r, material.diffuse_color.g, material.diffuse_color.b);
    glUniform3f(loc_material.ambient_color, material.ambient_color.r, material.ambient_color.g, material.ambient_color.b);
    glUniform3f(loc_material.specular_color, material.specular_color.r, material.specular_color.g, material.specular_color.b);
}

void LightingTechnique::set_point_lights(GLint num_lights, const PointLight* light) const {
    glUniform1i(loc_num_point_lights, num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        glUniform3f(loc_point_lights[i].color, light[i].color.r, light[i].color.g, light[i].color.b);
        glUniform1f(loc_point_lights[i].ambient_intensity, light[i].ambient_intensity);
        glUniform1f(loc_point_lights[i].diffuse_intensity, light[i].diffuse_intensity);
        const glm::vec3 local_pos = light[i].local_position();
        glUniform3f(loc_point_lights[i].position, local_pos.x, local_pos.y, local_pos.z);
        glUniform1f(loc_point_lights[i].atten.constant, light[i].attenuation.constant);
        glUniform1f(loc_point_lights[i].atten.linear, light[i].attenuation.linear);
        glUniform1f(loc_point_lights[i].atten.exp, light[i].attenuation.exp);
    }
}

void LightingTechnique::set_spot_lights(GLint num_lights, const SpotLight* light) const {
    glUniform1i(loc_num_spot_light, num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        glUniform3f(loc_spot_lights[i].color, light[i].color.r, light[i].color.g, light[i].color.b);
        glUniform1f(loc_spot_lights[i].ambient_intensity, light[i].ambient_intensity);
        glUniform1f(loc_spot_lights[i].diffuse_intensity, light[i].diffuse_intensity);
        const glm::vec3 local_pos = light[i].local_position();
        glUniform3f(loc_spot_lights[i].position, local_pos.x, local_pos.y, local_pos.z);
        const glm::vec3 local_dir = glm::normalize(light[i].local_direction());
        glUniform3f(loc_spot_lights[i].direction, local_dir.x, local_dir.y, local_dir.z);
        glUniform1f(loc_spot_lights[i].angle, cosf(glm::radians(light[i].angle)));
        glUniform1f(loc_spot_lights[i].atten.constant, light[i].attenuation.constant);
        glUniform1f(loc_spot_lights[i].atten.linear, light[i].attenuation.linear);
        glUniform1f(loc_spot_lights[i].atten.exp, light[i].attenuation.exp);
    }
}
