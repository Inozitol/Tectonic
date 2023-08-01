#include "shader/shadow/ShadowMapShader.h"


void ShadowMapShader::init() {
    Shader::init();
    addShader(GL_VERTEX_SHADER, "shaders/shadow.vs", nullptr);
    addShader(GL_FRAGMENT_SHADER, "shaders/shadow.fs", nullptr);
    finalize();

    loc_wvp                 = uniformLocation("u_WVP");
    loc_world               = uniformLocation("u_world");
    loc_light_world_pos     = uniformLocation("u_lightWorldPos");

    if(loc_wvp == 0xFFFFFFFF ||
       loc_world == 0xFFFFFFFF ||
       loc_light_world_pos == 0xFFFFFFFF){
        throw shaderException("Invalid uniform location");
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
