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

    loc_worldCameraPos = cacheUniform("u_worldCameraPos");

    loc_WVP = cacheUniform("u_WVP");
    loc_lightWVP = cacheUniform("u_LightWVP");
    loc_world = cacheUniform("u_world");

    loc_sampler.diffuse = cacheUniform("u_samplers.diffuse");
    loc_sampler.specular = cacheUniform("u_samplers.specular");
    loc_sampler.normal = cacheUniform("u_samplers.normal");
    loc_sampler.shadow_map = cacheUniform("u_samplers.shadowMap");
    loc_sampler.shadow_cube_map = cacheUniform("u_samplers.shadowCubeMap");

    loc_material.ambient_color = cacheUniform("u_material.ambientColor");
    loc_material.diffuse_color = cacheUniform("u_material.diffuseColor");
    loc_material.specular_color = cacheUniform("u_material.specularColor");
    loc_material.shininess = cacheUniform("u_material.shininess");

    loc_dirLight.color = cacheUniform("u_directionalLight.base.color");
    loc_dirLight.ambient_intensity = cacheUniform("u_directionalLight.base.ambientIntensity");
    loc_dirLight.diffuse_intensity = cacheUniform("u_directionalLight.base.diffuseIntensity");
    loc_dirLight.direction = cacheUniform("u_directionalLight.direction");

    loc_numPointLights = cacheUniform("u_pointLightsCount");
    loc_numSpotLights = cacheUniform("u_spotLightsCount");

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_pointLights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_pointLights[%d].base.color", i);
        loc_pointLights[i].color = cacheUniform(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].base.ambientIntensity", i);
        loc_pointLights[i].ambient_intensity = cacheUniform(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].base.diffuseIntensity", i);
        loc_pointLights[i].diffuse_intensity = cacheUniform(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].pos", i);
        loc_pointLights[i].position = cacheUniform(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.constant", i);
        loc_pointLights[i].atten.constant = cacheUniform(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.linear", i);
        loc_pointLights[i].atten.linear = cacheUniform(name);

        snprintf(name, sizeof(name), "u_pointLights[%d].atten.exp", i);
        loc_pointLights[i].atten.exp = cacheUniform(name);
    }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_spotLights); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_spotLights[%d].base.base.color", i);
        loc_spotLights[i].color = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.base.ambientIntensity", i);
        loc_spotLights[i].ambient_intensity = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.base.diffuseIntensity", i);
        loc_spotLights[i].diffuse_intensity = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.pos", i);
        loc_spotLights[i].position = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.atten.constant", i);
        loc_spotLights[i].atten.constant = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.atten.linear", i);
        loc_spotLights[i].atten.linear = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].base.atten.exp", i);
        loc_spotLights[i].atten.exp = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].direction", i);
        loc_spotLights[i].direction = cacheUniform(name);

        snprintf(name, sizeof(name), "u_spotLights[%d].angle", i);
        loc_spotLights[i].angle = cacheUniform(name);
    }

    loc_colorMod = cacheUniform("u_colorMod");
    loc_boneMatrixArray = cacheUniform("u_bonesMatrices", ShaderType::BONE_SHADER);
}

void LightingShader::setWVP(const glm::mat4x4 &wvp) const {
    glUniformMatrix4fv(getUniformLocation(loc_WVP), 1, GL_FALSE, glm::value_ptr(wvp));
}

void LightingShader::setLightWVP(const glm::mat4x4 &light_wvp) const {
    glUniformMatrix4fv(getUniformLocation(loc_lightWVP), 1, GL_FALSE, glm::value_ptr(light_wvp));
}

void LightingShader::setWorld(const glm::mat4 &world) const {
    glUniformMatrix4fv(getUniformLocation(loc_world), 1, GL_FALSE, glm::value_ptr(world));
}

void LightingShader::setDiffuseTextureUnit(GLint texUnit) const {
    glUniform1i(getUniformLocation(loc_sampler.diffuse), texUnit);
}

void LightingShader::setSpecularTextureUnit(GLint texUnit) const {
    glUniform1i(getUniformLocation(loc_sampler.specular), texUnit);
}

void LightingShader::setNormalTextureUnit(GLint texUnit) const {
    glUniform1i(getUniformLocation(loc_sampler.normal), texUnit);
}

void LightingShader::setShadowMapTextureUnit(GLint texUnit) const {
    glUniform1i(getUniformLocation(loc_sampler.shadow_map), texUnit);
}

void LightingShader::setShadowCubeMapTextureUnit(GLint texUnit) const {
    glUniform1i(getUniformLocation(loc_sampler.shadow_cube_map), texUnit);
}

void LightingShader::setDirectionalLight(const DirectionalLight &light) const {
    const glm::vec3 direction = light.getDirection();
    glUniform3f(getUniformLocation(loc_dirLight.color), light.color.r, light.color.g, light.color.b);
    glUniform1f(getUniformLocation(loc_dirLight.ambient_intensity), light.ambientIntensity);
    glUniform1f(getUniformLocation(loc_dirLight.diffuse_intensity), light.diffuseIntensity);
    glUniform3f(getUniformLocation(loc_dirLight.direction), direction.x, direction.y, direction.z);
}

void LightingShader::setWorldCameraPos(const glm::vec3 &pos) const {
    glUniform3f(getUniformLocation(loc_worldCameraPos), pos.x, pos.y, pos.z);
}

void LightingShader::setMaterial(const Material& material) const {
    glUniform3f(getUniformLocation(loc_material.diffuse_color),  material.m_diffuseColor.r,  material.m_diffuseColor.g,  material.m_diffuseColor.b);
    glUniform3f(getUniformLocation(loc_material.ambient_color),  material.m_ambientColor.r,  material.m_ambientColor.g,  material.m_ambientColor.b);
    glUniform3f(getUniformLocation(loc_material.specular_color), material.m_specularColor.r, material.m_specularColor.g, material.m_specularColor.b);
    glUniform1f(getUniformLocation(loc_material.shininess), material.m_shininess);
}

void LightingShader::setPointLights(GLint num_lights, const std::array<PointLight, MAX_POINT_LIGHTS>& light) const {
    glUniform1i(getUniformLocation(loc_numPointLights), num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        const glm::vec3 pos = light[i].getPosition();
        glUniform3f(getUniformLocation(loc_pointLights[i].color), light[i].color.r, light[i].color.g, light[i].color.b);
        glUniform1f(getUniformLocation(loc_pointLights[i].ambient_intensity), light[i].ambientIntensity);
        glUniform1f(getUniformLocation(loc_pointLights[i].diffuse_intensity), light[i].diffuseIntensity);
        glUniform3f(getUniformLocation(loc_pointLights[i].position), pos.x, pos.y, pos.z);
        glUniform1f(getUniformLocation(loc_pointLights[i].atten.constant), light[i].attenuation.constant);
        glUniform1f(getUniformLocation(loc_pointLights[i].atten.linear), light[i].attenuation.linear);
        glUniform1f(getUniformLocation(loc_pointLights[i].atten.exp), light[i].attenuation.exp);
    }
}

void LightingShader::setSpotLights(GLint num_lights, const std::array<SpotLight, MAX_SPOT_LIGHTS>& light) const {
    glUniform1i(getUniformLocation(loc_numSpotLights), num_lights);
    for(int32_t i = 0; i < num_lights; i++){
        const glm::vec3 worldPos = light[i].getPosition();
        const glm::vec3 worldDir = glm::normalize(light[i].getDirection());
        glUniform3f(getUniformLocation(loc_spotLights[i].color), light[i].color.r, light[i].color.g, light[i].color.b);
        glUniform1f(getUniformLocation(loc_spotLights[i].ambient_intensity), light[i].ambientIntensity);
        glUniform1f(getUniformLocation(loc_spotLights[i].diffuse_intensity), light[i].diffuseIntensity);
        glUniform3f(getUniformLocation(loc_spotLights[i].position), worldPos.x, worldPos.y, worldPos.z);
        glUniform3f(getUniformLocation(loc_spotLights[i].direction), worldDir.x, worldDir.y, worldDir.z);
        glUniform1f(getUniformLocation(loc_spotLights[i].angle), cosf(glm::radians(light[i].angle)));
        glUniform1f(getUniformLocation(loc_spotLights[i].atten.constant), light[i].attenuation.constant);
        glUniform1f(getUniformLocation(loc_spotLights[i].atten.linear), light[i].attenuation.linear);
        glUniform1f(getUniformLocation(loc_spotLights[i].atten.exp), light[i].attenuation.exp);
    }
}

void LightingShader::setColorMod(const glm::vec4 &clr) const {
    glUniform4f(getUniformLocation(loc_colorMod), clr.r, clr.g, clr.b, clr.a);
}

void LightingShader::setBoneTransforms(const boneTransfoms_t &transforms) const {
    glUniformMatrix4fv(getUniformLocation(loc_boneMatrixArray), MAX_BONES, GL_FALSE, glm::value_ptr(transforms[0]));
}

