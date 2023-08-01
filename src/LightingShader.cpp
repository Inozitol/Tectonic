#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include "shader/LightingShader.h"

#include "utils.h"

void DirectionalLight::calcLocalDirection(Transformation& transform) {
    glm::mat3x3 world3(transform.getMatrix());
    glm::mat3x3 world2local = glm::transpose(world3);   // Assuming uniform scaling
    m_localDirection = glm::normalize(world2local * getDirection());
}

DirectionalLight::DirectionalLight() {
    m_lightView.switchOrthographic();
    m_lightView.setOrthographicInfo(shadowOrthoInfo);
    m_lightView.createProjectionMatrix();
}

void DirectionalLight::updateTightOrthoProjection(const Camera& gameCamera) {
    shadowOrthoInfo = Utils::createTightOrthographicInfo(m_lightView, gameCamera);
    m_lightView.setOrthographicInfo(shadowOrthoInfo);
    m_lightView.createProjectionMatrix();
}

void PointLight::calcLocalPosition(Transformation& transform) {
    m_localPos = transform.invertPosition(getWorldPosition());
}

PointLight::PointLight() {
    m_lightView.switchPerspective();
    m_lightView.setPerspectiveInfo(shadowPersInfo);
    m_lightView.createProjectionMatrix();
}

void SpotLight::calcLocalDirectionPosition(Transformation &transform) {
    PointLight::calcLocalPosition(transform);
    m_localDirection = transform.invertDirection(m_lightView.getDirection());
}

SpotLight::SpotLight() {
    m_lightView.switchPerspective();
    m_lightView.setPerspectiveInfo(shadowPersInfo);
    m_lightView.createProjectionMatrix();
}

void LightingShader::init() {

    Shader::init();

    std::string defines;

    if(!Utils::readFile("include/defs/ShaderDefines.h", defines)){
        throw shaderException("Unable to load ShaderDefines.h file.");
    }

    addShader(GL_VERTEX_SHADER, "shaders/lighting.vs", defines.c_str());
    addShader(GL_FRAGMENT_SHADER, "shaders/lighting.fs", defines.c_str());
    finalize();

    loc_local_camera_pos            = uniformLocation("u_localCameraPos");

    loc_wvp                         = uniformLocation("u_WVP");
    loc_light_wvp                   = uniformLocation("u_LightWVP");
    loc_world                       = uniformLocation("u_world");

    loc_sampler.diffuse             = uniformLocation("u_samplers.diffuse");
    loc_sampler.specular            = uniformLocation("u_samplers.specular");
    loc_sampler.shadow_map          = uniformLocation("u_samplers.shadowMap");
    loc_sampler.shadow_cube_map     = uniformLocation("u_samplers.shadowCubeMap");

    loc_material.ambient_color      = uniformLocation("u_material.ambientColor");
    loc_material.diffuse_color      = uniformLocation("u_material.diffuseColor");
    loc_material.specular_color     = uniformLocation("u_material.specularColor");

    loc_dir_light.color             = uniformLocation("u_directionalLight.base.color");
    loc_dir_light.ambient_intensity = uniformLocation("u_directionalLight.base.ambientIntensity");
    loc_dir_light.diffuse_intensity = uniformLocation("u_directionalLight.base.diffuseIntensity");
    loc_dir_light.direction         = uniformLocation("u_directionalLight.direction");

    loc_num_point_lights            = uniformLocation("u_pointLightsCount");
    loc_num_spot_light              = uniformLocation("u_spotLightsCount");

    if(loc_wvp == 0xFFFFFFFF ||
       loc_sampler.diffuse == 0xFFFFFFFF ||
       loc_sampler.specular == 0xFFFFFFFF ||
       loc_sampler.shadow_map == 0xFFFFFFFF ||
       loc_material.ambient_color == 0xFFFFFFFF ||
       loc_material.diffuse_color == 0xFFFFFFFF ||
       loc_material.specular_color == 0xFFFFFFFF ||
       loc_dir_light.color == 0xFFFFFFFF ||
       loc_dir_light.ambient_intensity == 0xFFFFFFFF ||
       loc_dir_light.diffuse_intensity == 0xFFFFFFFF ||
       loc_dir_light.direction == 0xFFFFFFFF ||
       loc_num_point_lights == 0xFFFFFFFF){
        throw shaderException("Invalid uniform locations");
    }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_point_lights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_pointLights[%d].base.color", i);
        loc_point_lights[i].color = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].base.ambientIntensity", i);
        loc_point_lights[i].ambient_intensity = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].base.diffuseIntensity", i);
        loc_point_lights[i].diffuse_intensity = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].localPos", i);
        loc_point_lights[i].local_position = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].worldPos", i);
        loc_point_lights[i].world_position = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.constant", i);
        loc_point_lights[i].atten.constant = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.linear", i);
        loc_point_lights[i].atten.linear = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.exp", i);
        loc_point_lights[i].atten.exp = uniformLocation(name);

        if(loc_point_lights[i].color == 0xFFFFFFFF ||
           loc_point_lights[i].ambient_intensity == 0xFFFFFFFF ||
           loc_point_lights[i].diffuse_intensity == 0xFFFFFFFF ||
           loc_point_lights[i].local_position == 0xFFFFFFFF ||
           loc_point_lights[i].atten.constant == 0xFFFFFFFF ||
           loc_point_lights[i].atten.linear == 0xFFFFFFFF ||
           loc_point_lights[i].atten.exp == 0xFFFFFFFF){
            throw shaderException("Invalid uniform locations");
            }
        }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_spot_lights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_spotLights[%d].base.base.color", i);
        loc_spot_lights[i].color = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.base.ambientIntensity", i);
        loc_spot_lights[i].ambient_intensity = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.base.diffuseIntensity", i);
        loc_spot_lights[i].diffuse_intensity = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.localPos", i);
        loc_spot_lights[i].position = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.atten.constant", i);
        loc_spot_lights[i].atten.constant = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.atten.linear", i);
        loc_spot_lights[i].atten.linear = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.atten.exp", i);
        loc_spot_lights[i].atten.exp = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].direction", i);
        loc_spot_lights[i].direction = uniformLocation(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].angle", i);
        loc_spot_lights[i].angle = uniformLocation(name);

        if(loc_spot_lights[i].color == 0xFFFFFFFF ||
           loc_spot_lights[i].ambient_intensity == 0xFFFFFFFF ||
           loc_spot_lights[i].diffuse_intensity == 0xFFFFFFFF ||
           loc_spot_lights[i].position == 0xFFFFFFFF ||
           loc_spot_lights[i].atten.constant == 0xFFFFFFFF ||
           loc_spot_lights[i].atten.linear == 0xFFFFFFFF ||
           loc_spot_lights[i].atten.exp == 0xFFFFFFFF ||
           loc_spot_lights[i].direction == 0xFFFFFFFF ||
           loc_spot_lights[i].angle == 0xFFFFFFFF){
            throw shaderException("Invalid uniform locations");
        }
    }

}

void LightingShader::setWVP(const glm::mat4x4 &wvp) const {
    glUniformMatrix4fv(loc_wvp, 1, GL_FALSE, glm::value_ptr(wvp));
}

void LightingShader::setLightWVP(const glm::mat4x4 &light_wvp) const {
    glUniformMatrix4fv(loc_light_wvp, 1, GL_FALSE, glm::value_ptr(light_wvp));
}

void LightingShader::setWorld(const glm::mat4 &world) const {
    glUniformMatrix4fv(loc_world, 1, GL_FALSE, glm::value_ptr(world));
}

void LightingShader::setDiffuseTextureUnit(GLint tex_unit) const {
    glUniform1i(loc_sampler.diffuse, tex_unit);
}

void LightingShader::setSpecularTextureUnit(GLint tex_unit) const{
    glUniform1i(loc_sampler.specular, tex_unit);
}

void LightingShader::setShadowMapTextureUnit(GLint tex_unit) const{
    glUniform1i(loc_sampler.shadow_map, tex_unit);
}

void LightingShader::setShadowCubeMapTextureUnit(GLint tex_unit) const {
    glUniform1i(loc_sampler.shadow_cube_map, tex_unit);
}

void LightingShader::setDirectionalLight(const DirectionalLight &light) const {
    glUniform3f(loc_dir_light.color, light.color.r, light.color.g, light.color.b);
    glUniform1f(loc_dir_light.diffuse_intensity, light.diffuseIntensity);
    const glm::vec3 local_direction = light.localDirection();
    glUniform3f(loc_dir_light.direction, local_direction.x, local_direction.y, local_direction.z);
    glUniform1f(loc_dir_light.ambient_intensity, light.ambientIntensity);
}

void LightingShader::setLocalCameraPos(const glm::vec3 &pos) const {
    glUniform3f(loc_local_camera_pos, pos.x, pos.y, pos.z);
}

void LightingShader::setMaterial(const Material &material) const {
    glUniform3f(loc_material.diffuse_color, material.diffuseColor.r, material.diffuseColor.g, material.diffuseColor.b);
    glUniform3f(loc_material.ambient_color, material.ambientColor.r, material.ambientColor.g, material.ambientColor.b);
    glUniform3f(loc_material.specular_color, material.specularColor.r, material.specularColor.g, material.specularColor.b);
}

void LightingShader::setPointLights(GLint num_lights, const std::shared_ptr<PointLight> *light) const {
    glUniform1i(loc_num_point_lights, num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        glUniform3f(loc_point_lights[i].color, light[i]->color.r, light[i]->color.g, light[i]->color.b);
        glUniform1f(loc_point_lights[i].ambient_intensity, light[i]->ambientIntensity);
        glUniform1f(loc_point_lights[i].diffuse_intensity, light[i]->diffuseIntensity);
        const glm::vec3 local_pos = light[i]->getLocalPosition();
        glUniform3f(loc_point_lights[i].local_position, local_pos.x, local_pos.y, local_pos.z);
        const glm::vec3 world_pos = light[i]->getWorldPosition();
        glUniform3f(loc_point_lights[i].world_position, world_pos.x, world_pos.y, world_pos.z);
        glUniform1f(loc_point_lights[i].atten.constant, light[i]->attenuation.constant);
        glUniform1f(loc_point_lights[i].atten.linear, light[i]->attenuation.linear);
        glUniform1f(loc_point_lights[i].atten.exp, light[i]->attenuation.exp);
    }
}

void LightingShader::setSpotLights(GLint num_lights, const std::shared_ptr<SpotLight> *light) const {
    glUniform1i(loc_num_spot_light, num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        glUniform3f(loc_spot_lights[i].color, light[i]->color.r, light[i]->color.g, light[i]->color.b);
        glUniform1f(loc_spot_lights[i].ambient_intensity, light[i]->ambientIntensity);
        glUniform1f(loc_spot_lights[i].diffuse_intensity, light[i]->diffuseIntensity);
        const glm::vec3 local_pos = light[i]->getLocalPosition();
        glUniform3f(loc_spot_lights[i].position, local_pos.x, local_pos.y, local_pos.z);
        const glm::vec3 local_dir = glm::normalize(light[i]->localDirection());
        glUniform3f(loc_spot_lights[i].direction, local_dir.x, local_dir.y, local_dir.z);
        glUniform1f(loc_spot_lights[i].angle, cosf(glm::radians(light[i]->angle)));
        glUniform1f(loc_spot_lights[i].atten.constant, light[i]->attenuation.constant);
        glUniform1f(loc_spot_lights[i].atten.linear, light[i]->attenuation.linear);
        glUniform1f(loc_spot_lights[i].atten.exp, light[i]->attenuation.exp);
    }
}
