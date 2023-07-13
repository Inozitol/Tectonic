#include "ShadowMapShader.h"


void ShadowMapShader::init() {
    Shader::init();
    addShader(GL_VERTEX_SHADER, "shaders/shadow.vs", nullptr);
    addShader(GL_FRAGMENT_SHADER, "shaders/empty.fs", nullptr);
    finalize();

    loc_wvp = uniformLocation("wvp");
    if(loc_wvp == 0xFFFFFFFF){
        throw shaderException("Invalid uniform location");
    }
}

void ShadowMapShader::setWVP(const glm::mat4x4 &wvp) const {
    glUniformMatrix4fv(loc_wvp, 1, GL_FALSE, glm::value_ptr(wvp));
}
