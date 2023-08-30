#ifndef TECTONIC_SHADOWMAPSHADER_H
#define TECTONIC_SHADOWMAPSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"
#include "shader/Shader.h"
#include "defs/ConfigDefs.h"
#include "defs/ShaderDefines.h"

class ShadowMapShader : public Shader {
public:
    ShadowMapShader() = default;
    void init() override;
    void setWVP(const glm::mat4& wvp) const;
    void setWorld(const glm::mat4& world) const;
    void setLightWorldPos(const glm::vec3& pos) const;
    void setBoneTransform(uint32_t boneId, glm::mat4 transform) const;

private:
    GLint loc_wvp = -1;
    GLint loc_world = -1;
    GLint loc_light_world_pos = -1;
    GLint loc_bone[MAX_BONES] = {-1};

};

#endif //TECTONIC_SHADOWMAPSHADER_H
