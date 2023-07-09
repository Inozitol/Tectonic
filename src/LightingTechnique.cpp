#include "LightingTechnique.h"

void DirectionalLight::calc_local_direction(const glm::mat4x4 &world) {
    glm::mat3x3 world3(world);
    glm::mat3x3 world2local = glm::transpose(world3);

    _local_direction = glm::normalize(world2local * direction);
}

LightingTechnique::LightingTechnique() {}

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

    loc_material.diffuse_color      = uniform_location("material.diffuse_color");
    loc_material.ambient_color      = uniform_location("material.ambient_color");
    loc_material.specular_color      = uniform_location("material.specular_color");

    loc_dir_light.color             = uniform_location("directional_light.color");
    loc_dir_light.ambient_intensity = uniform_location("directional_light.ambient_intensity");
    loc_dir_light.diffuse_intensity = uniform_location("directional_light.diffuse_intensity");
    loc_dir_light.direction         = uniform_location("directional_light.direction");

    if(loc_wvp.world == 0xFFFFFFFF ||
       loc_wvp.view == 0xFFFFFFFF ||
       loc_wvp.projection == 0xFFFFFFFF ||
       loc_sampler.diffuse == 0xFFFFFFFF ||
       loc_sampler.specular == 0xFFFFFFFF ||
       loc_material.diffuse_color == 0xFFFFFFFF ||
       loc_material.ambient_color == 0xFFFFFFFF ||
       loc_dir_light.color == 0xFFFFFFFF ||
       loc_dir_light.ambient_intensity == 0xFFFFFFFF ||
       loc_dir_light.direction == 0xFFFFFFFF ||
       loc_dir_light.diffuse_intensity == 0xFFFFFFFF){
        throw technique_exception("Invalid uniform locations");
    }

}

void LightingTechnique::set_world(const glm::mat4x4 &world) const {
    glUniformMatrix4fv(loc_wvp.world, 1, GL_FALSE, &world[0][0]);
}

void LightingTechnique::set_view(const glm::mat4x4 &view) const {
    glUniformMatrix4fv(loc_wvp.view, 1, GL_FALSE, &view[0][0]);
}

void LightingTechnique::set_projection(const glm::mat4x4& projection) const {
    glUniformMatrix4fv(loc_wvp.projection, 1, GL_FALSE, &projection[0][0]);
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
