#include "shader/DebugShader.h"

void DebugShader::init() {
    Shader::init();

    addShader(GL_VERTEX_SHADER, DEBUG_VERT_SHADER_PATH);
    addShader(GL_GEOMETRY_SHADER, DEBUG_GEOM_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, DEBUG_FRAG_SHADER_PATH);
    finalize();

    loc_wvp = uniformLocation("u_WVP");
    loc_world = uniformLocation("u_world");
    for(uint32_t i = 0; i < ARRAY_SIZE(loc_bone); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_bonesMatrices[%d]", i);
        loc_bone[i] = uniformLocation(name);
    }

}

void DebugShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(loc_wvp, 1, GL_FALSE, glm::value_ptr(wvp));
}

void DebugShader::setWorld(const glm::mat4 &world) const {
    glUniformMatrix4fv(loc_world, 1, GL_FALSE, glm::value_ptr(world));
}

void DebugShader::setBoneTransform(uint32_t boneId, const glm::mat4 &transform) const {
    glUniformMatrix4fv(loc_bone[boneId], 1, GL_FALSE, glm::value_ptr(transform));
}
