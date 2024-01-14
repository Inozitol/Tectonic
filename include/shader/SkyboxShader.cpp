#include "SkyboxShader.h"

void SkyboxShader::init() {
    Shader::init();

    addShader(GL_VERTEX_SHADER, SKYBOX_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, SKYBOX_FRAG_SHADER_PATH);
    finalize();

    loc_VP = cacheUniform("u_VP");
    loc_sampler.cubemap = cacheUniform("u_skybox");
}

void SkyboxShader::setVP(const glm::mat4 &vp) const {
    glUniformMatrix4fv(getUniformLocation(loc_VP), 1, GL_FALSE, glm::value_ptr(vp));
}

void SkyboxShader::setCubemapUnit(GLint texUnit) {
    glUniform1i(getUniformLocation(loc_sampler.cubemap), texUnit);
}