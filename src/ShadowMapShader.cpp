#include "shader/shadow/ShadowMapShader.h"

void ShadowMapShader::init() {
    Shader::init();
    addShader(GL_VERTEX_SHADER, SHADOWMAP_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, SHADOWMAP_FRAG_SHADER_PATH);
    finalize();

    loc_wvp = cacheUniform("u_WVP");
    loc_world = cacheUniform("u_world");
    loc_light_world_pos = cacheUniform("u_lightWorldPos");
    loc_boneMatrixArray = cacheUniform("u_bonesMatrices", ShaderType::BONE_SHADER);
}

void ShadowMapShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(getUniformLocation(loc_wvp), 1, GL_FALSE, glm::value_ptr(wvp));
}

void ShadowMapShader::setWorld(const glm::mat4 &world) const {
    glUniformMatrix4fv(getUniformLocation(loc_world), 1, GL_FALSE, glm::value_ptr(world));
}

void ShadowMapShader::setLightWorldPos(const glm::vec3 &pos) const {
    glUniform3f(getUniformLocation(loc_light_world_pos), pos.x, pos.y, pos.z);
}

void ShadowMapShader::setBoneTransforms(const boneTransfoms_t &transforms) const {
    glUniformMatrix4fv(getUniformLocation(loc_boneMatrixArray), MAX_BONES, GL_FALSE, glm::value_ptr(transforms[0]));
}
