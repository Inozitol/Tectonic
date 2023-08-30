#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <array>
#include "shader/LightingShader.h"

#include "utils.h"

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

PointLight::PointLight() {
    m_lightView.switchPerspective();
    m_lightView.setPerspectiveInfo(shadowPersInfo);
    m_lightView.createProjectionMatrix();
}

SpotLight::SpotLight() {
    m_lightView.switchPerspective();
    m_lightView.setPerspectiveInfo(shadowPersInfo);
    m_lightView.createProjectionMatrix();
}

void LightingShader::init() {

    Shader::init();

    addShader(GL_VERTEX_SHADER, LIGHTING_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, LIGHTING_FRAG_SHADER_PATH);
    finalize();

    loc_worldCameraPos            = uniformLocation("u_worldCameraPos");

    loc_wvp                         = uniformLocation("u_WVP");
    loc_light_wvp                   = uniformLocation("u_LightWVP");
    loc_world                       = uniformLocation("u_world");

    loc_sampler.diffuse             = uniformLocation("u_samplers.diffuse");
    loc_sampler.specular            = uniformLocation("u_samplers.specular");
    loc_sampler.normal              = uniformLocation("u_samplers.normal");
    loc_sampler.shadow_map          = uniformLocation("u_samplers.shadowMap");
    loc_sampler.shadow_cube_map     = uniformLocation("u_samplers.shadowCubeMap");

    loc_material.ambient_color      = uniformLocation("u_material.ambientColor");
    loc_material.diffuse_color      = uniformLocation("u_material.diffuseColor");
    loc_material.specular_color     = uniformLocation("u_material.specularColor");
    loc_material.shininess          = uniformLocation("u_material.shininess");

    loc_dir_light.color             = uniformLocation("u_directionalLight.base.color");
    loc_dir_light.ambient_intensity = uniformLocation("u_directionalLight.base.ambientIntensity");
    loc_dir_light.diffuse_intensity = uniformLocation("u_directionalLight.base.diffuseIntensity");
    loc_dir_light.direction         = uniformLocation("u_directionalLight.direction");

    loc_num_point_lights            = uniformLocation("u_pointLightsCount");
    loc_num_spot_light              = uniformLocation("u_spotLightsCount");

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_point_lights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_pointLights[%d].base.color", i);
        loc_point_lights[i].color = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].base.ambientIntensity", i);
        loc_point_lights[i].ambient_intensity = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].base.diffuseIntensity", i);
        loc_point_lights[i].diffuse_intensity = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].pos", i);
        loc_point_lights[i].position = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.constant", i);
        loc_point_lights[i].atten.constant = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.linear", i);
        loc_point_lights[i].atten.linear = uniformLocation(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.exp", i);
        loc_point_lights[i].atten.exp = uniformLocation(name);
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

        snprintf(name, sizeof(name), "u_spotLights[%d].base.pos", i);
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
    }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_bone); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_bonesMatrices[%d]", i);
        loc_bone[i] = uniformLocation(name);
    }

    loc_colorMod = uniformLocation("u_colorMod");
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

void LightingShader::setNormalTextureUnit(GLint tex_unit) const {
    glUniform1i(loc_sampler.normal, tex_unit);
}

void LightingShader::setShadowMapTextureUnit(GLint tex_unit) const{
    glUniform1i(loc_sampler.shadow_map, tex_unit);
}

void LightingShader::setShadowCubeMapTextureUnit(GLint tex_unit) const {
    glUniform1i(loc_sampler.shadow_cube_map, tex_unit);
}

void LightingShader::setDirectionalLight(const DirectionalLight &light) const {
    glUniform3f(loc_dir_light.color, light.color.r, light.color.g, light.color.b);
    glUniform1f(loc_dir_light.ambient_intensity, light.ambientIntensity);
    glUniform1f(loc_dir_light.diffuse_intensity, light.diffuseIntensity);
    const glm::vec3 direction = light.getDirection();
    glUniform3f(loc_dir_light.direction, direction.x, direction.y, direction.z);
}

void LightingShader::setWorldCameraPos(const glm::vec3 &pos) const {
    glUniform3f(loc_worldCameraPos, pos.x, pos.y, pos.z);
}

void LightingShader::setMaterial(const Material& material) const {
    glUniform3f(loc_material.diffuse_color,  material.m_diffuseColor.r,  material.m_diffuseColor.g,  material.m_diffuseColor.b);
    glUniform3f(loc_material.ambient_color,  material.m_ambientColor.r,  material.m_ambientColor.g,  material.m_ambientColor.b);
    glUniform3f(loc_material.specular_color, material.m_specularColor.r, material.m_specularColor.g, material.m_specularColor.b);
    glUniform1f(loc_material.shininess, material.m_shininess);
}

void LightingShader::setPointLights(GLint num_lights, const std::array<PointLight, MAX_POINT_LIGHTS>& light) const {
    glUniform1i(loc_num_point_lights, num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        glUniform3f(loc_point_lights[i].color, light[i].color.r, light[i].color.g, light[i].color.b);
        glUniform1f(loc_point_lights[i].ambient_intensity, light[i].ambientIntensity);
        glUniform1f(loc_point_lights[i].diffuse_intensity, light[i].diffuseIntensity);
        const glm::vec3 pos = light[i].getPosition();
        glUniform3f(loc_point_lights[i].position, pos.x, pos.y, pos.z);
        glUniform1f(loc_point_lights[i].atten.constant, light[i].attenuation.constant);
        glUniform1f(loc_point_lights[i].atten.linear, light[i].attenuation.linear);
        glUniform1f(loc_point_lights[i].atten.exp, light[i].attenuation.exp);
    }
}

void LightingShader::setSpotLights(GLint num_lights, const std::array<SpotLight, MAX_SPOT_LIGHTS>& light) const {
    glUniform1i(loc_num_spot_light, num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        glUniform3f(loc_spot_lights[i].color, light[i].color.r, light[i].color.g, light[i].color.b);
        glUniform1f(loc_spot_lights[i].ambient_intensity, light[i].ambientIntensity);
        glUniform1f(loc_spot_lights[i].diffuse_intensity, light[i].diffuseIntensity);
        const glm::vec3 worldPos = light[i].getPosition();
        glUniform3f(loc_spot_lights[i].position, worldPos.x, worldPos.y, worldPos.z);
        const glm::vec3 worldDir = glm::normalize(light[i].getDirection());
        glUniform3f(loc_spot_lights[i].direction, worldDir.x, worldDir.y, worldDir.z);
        glUniform1f(loc_spot_lights[i].angle, cosf(glm::radians(light[i].angle)));
        glUniform1f(loc_spot_lights[i].atten.constant, light[i].attenuation.constant);
        glUniform1f(loc_spot_lights[i].atten.linear, light[i].attenuation.linear);
        glUniform1f(loc_spot_lights[i].atten.exp, light[i].attenuation.exp);
    }
}

void LightingShader::setBoneTransform(uint32_t boneId, glm::mat4 transform) const {
    glUniformMatrix4fv(loc_bone[boneId], 1, GL_FALSE, glm::value_ptr(transform));
}

void LightingShader::setColorMod(const glm::vec4 &clr) const {
    glUniform4f(loc_colorMod, clr.r, clr.g, clr.b, clr.a);
}
