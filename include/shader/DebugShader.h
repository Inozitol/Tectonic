#ifndef TECTONIC_DEBUGSHADER_H
#define TECTONIC_DEBUGSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "defs/ShaderDefines.h"
#include "defs/ConfigDefs.h"
#include "utils.h"

class DebugShader : public Shader{
public:
    DebugShader() = default;
    void init() override;
    void setWVP(const glm::mat4& wvp) const;
    void setWorld(const glm::mat4& world) const;
    void setBoneTransform(uint32_t boneId, const glm::mat4& transform) const;

private:
    // WVP matrix
    GLint loc_wvp = -1;

    // World matrix
    GLint loc_world = -1;

    // Array of bones inside the scene
    GLint loc_bone[MAX_BONES] = {-1};

};


#endif //TECTONIC_DEBUGSHADER_H
