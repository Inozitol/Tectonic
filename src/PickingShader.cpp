#include <glm/gtc/type_ptr.hpp>
#include "shader/PickingShader.h"

void PickingShader::init() {
    Shader::init();

    addShader(GL_VERTEX_SHADER, PICKING_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, PICKING_FRAG_SHADER_PATH);
    finalize();

    loc_WVP         = uniformLocation("u_WVP");
    loc_drawIndex   = uniformLocation("u_drawIndex");
    loc_objectIndex = uniformLocation("u_objectIndex");

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_bone); i++){
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_bonesMatrices[%d]", i);
        loc_bone[i] = uniformLocation(name);
    }

}

void PickingShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(loc_WVP, 1, GL_FALSE, glm::value_ptr(wvp));
}

void PickingShader::setObjectIndex(uint32_t objectIndex) const {
    glUniform1ui(loc_objectIndex, objectIndex);
}

void PickingShader::drawStartCB(uint32_t drawIndex) {
    glUniform1ui(loc_drawIndex, drawIndex);
}

void PickingShader::setBoneTransform(uint32_t boneId, glm::mat4 transform) const {
    glUniformMatrix4fv(loc_bone[boneId], 1, GL_FALSE, glm::value_ptr(transform));
}
