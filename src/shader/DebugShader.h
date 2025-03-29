#ifndef TECTONIC_DEBUGSHADER_H
#define TECTONIC_DEBUGSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "defs/ShaderDefines.h"
#include "defs/ConfigDefs.h"
#include "utils/utils.h"
#include "model/ModelTypes.h"

class DebugShader : public Shader{
public:
    DebugShader() : Shader(ShaderType::BASIC_SHADER | ShaderType::BONE_SHADER){}
    void init() override;
    void setWVP(const glm::mat4& wvp) const;
    void setWorld(const glm::mat4& world) const;
    void setBoneTransforms(const boneTransfoms_t& transforms) const;

private:
    // WVP matrix
    uint32_t loc_WVP = -1;

    // World matrix
    uint32_t loc_world = -1;

    // Array of bones inside the scene
    uint32_t loc_boneMatrixArray{};
};


#endif //TECTONIC_DEBUGSHADER_H
