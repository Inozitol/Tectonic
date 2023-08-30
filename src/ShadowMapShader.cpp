#include "shader/shadow/ShadowMapShader.h"

void ShadowMapShader::init() {
    Shader::init();
    addShader(GL_VERTEX_SHADER, SHADOWMAP_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, SHADOWMAP_FRAG_SHADER_PATH);
    finalize();

    loc_wvp                 = uniformLocation("u_WVP");
    loc_world               = uniformLocation("u_world");
    loc_light_world_pos     = uniformLocation("u_lightWorldPos");

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_bone); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_bonesMatrices[%d]", i);
        loc_bone[i] = uniformLocation(name);
    }
}

void ShadowMapShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(loc_wvp, 1, GL_FALSE, glm::value_ptr(wvp));
}

void ShadowMapShader::setWorld(const glm::mat4 &world) const {
    glUniformMatrix4fv(loc_world, 1, GL_FALSE, glm::value_ptr(world));
}

void ShadowMapShader::setLightWorldPos(const glm::vec3 &pos) const {
    glUniform3f(loc_light_world_pos, pos.x, pos.y, pos.z);
}

void ShadowMapShader::setBoneTransform(uint32_t boneId, glm::mat4 transform) const {
    glUniformMatrix4fv(loc_bone[boneId], 1, GL_FALSE, glm::value_ptr(transform));
}
