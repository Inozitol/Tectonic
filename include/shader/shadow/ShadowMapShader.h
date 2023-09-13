#ifndef TECTONIC_SHADOWMAPSHADER_H
#define TECTONIC_SHADOWMAPSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"
#include "shader/Shader.h"
#include "defs/ConfigDefs.h"
#include "defs/ShaderDefines.h"
#include "model/ModelTypes.h"

class ShadowMapShader : public Shader {
public:
    ShadowMapShader() : Shader(ShaderType::BASIC_SHADER | ShaderType::BONE_SHADER){}
    void init() override;
    void setWVP(const glm::mat4& wvp) const;
    void setWorld(const glm::mat4& world) const;
    void setLightWorldPos(const glm::vec3& pos) const;
    void setBoneTransforms(const boneTransfoms_t& transforms) const;

private:
    uint32_t loc_wvp = -1;
    uint32_t loc_world = -1;
    uint32_t loc_light_world_pos = -1;
    uint32_t loc_boneMatrixArray{};
};

#endif //TECTONIC_SHADOWMAPSHADER_H
