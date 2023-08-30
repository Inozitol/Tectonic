#ifndef TECTONIC_PICKINGSHADER_H
#define TECTONIC_PICKINGSHADER_H

#include "utils.h"
#include "shader/Shader.h"
#include "defs/ShaderDefines.h"
#include "defs/ConfigDefs.h"

class PickingShader : public Shader {
public:
    PickingShader() = default;
    void init() override;

    void setWVP(const glm::mat4& wvp) const;
    void setObjectIndex(uint32_t objectIndex) const;
    void drawStartCB(uint32_t drawIndex);
    void setBoneTransform(uint32_t boneId, glm::mat4 transform) const;

private:
    GLint loc_WVP = -1;
    GLint loc_drawIndex = -1;
    GLint loc_objectIndex = -1;
    GLint loc_bone[MAX_BONES] = {-1};
};


#endif //TECTONIC_PICKINGSHADER_H
