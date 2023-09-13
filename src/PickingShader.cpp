#include <glm/gtc/type_ptr.hpp>
#include "shader/PickingShader.h"

void PickingShader::init() {
    Shader::init();

    addShader(GL_VERTEX_SHADER, PICKING_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, PICKING_FRAG_SHADER_PATH);
    finalize();

    loc_WVP = cacheUniform("u_WVP");
    loc_objectIndex = cacheUniform("u_objectIndex");
    loc_objectFlags = cacheUniform("u_objectFlags");
    loc_boneMatrixArray = cacheUniform("u_bonesMatrices", ShaderType::BONE_SHADER);
}

void PickingShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(getUniformLocation(loc_WVP), 1, GL_FALSE, glm::value_ptr(wvp));
}

void PickingShader::setObjectIndex(uint32_t objectIndex) const {
    glUniform1ui(getUniformLocation(loc_objectIndex), objectIndex);

}

void PickingShader::setObjectFlags(uint32_t flags) const {
    glUniform1ui(getUniformLocation(loc_objectFlags), flags);
}

void PickingShader::setBoneTransforms(const boneTransfoms_t &transforms) const {
    glUniformMatrix4fv(getUniformLocation(loc_boneMatrixArray), MAX_BONES, GL_FALSE, glm::value_ptr(transforms[0]));
}
