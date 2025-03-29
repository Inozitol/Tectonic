#ifndef TECTONIC_PICKINGSHADER_H
#define TECTONIC_PICKINGSHADER_H

#include "shader/Shader.h"
#include "defs/ShaderDefines.h"
#include "defs/ConfigDefs.h"
#include "model/ModelTypes.h"
#include "utils/utils.h"

class PickingShader : public Shader {
public:
    PickingShader() : Shader(ShaderType::BASIC_SHADER | ShaderType::BONE_SHADER){}
    void init() override;

    void setWVP(const glm::mat4& wvp) const;
    void setObjectIndex(uint32_t objectIndex) const;
    void setObjectFlags(uint32_t flags) const;
    void setBoneTransforms(const boneTransfoms_t& transforms) const;

private:
    uint32_t loc_WVP = -1;
    uint32_t loc_objectIndex = -1;
    uint32_t loc_objectFlags = -1;
    uint32_t loc_boneMatrixArray = -1;

};


#endif //TECTONIC_PICKINGSHADER_H
