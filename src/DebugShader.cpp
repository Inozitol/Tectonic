#include "shader/DebugShader.h"

void DebugShader::init() {
    Shader::init();

    addShader(GL_VERTEX_SHADER, DEBUG_VERT_SHADER_PATH);
    addShader(GL_GEOMETRY_SHADER, DEBUG_GEOM_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, DEBUG_FRAG_SHADER_PATH);
    finalize();

    loc_WVP = cacheUniform("u_WVP");
    loc_world = cacheUniform("u_world");
    loc_boneMatrixArray = cacheUniform("u_bonesMatrices", ShaderType::BONE_SHADER);
}

void DebugShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(getUniformLocation(loc_WVP), 1, GL_FALSE, glm::value_ptr(wvp));
}

void DebugShader::setWorld(const glm::mat4 &world) const {
    glUniformMatrix4fv(getUniformLocation(loc_world), 1, GL_FALSE, glm::value_ptr(world));
}

void DebugShader::setBoneTransforms(const boneTransfoms_t &transforms) const {
    glUniformMatrix4fv(getUniformLocation(loc_boneMatrixArray), MAX_BONES, GL_FALSE, glm::value_ptr(transforms[0]));
}
