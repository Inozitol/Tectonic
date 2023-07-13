#ifndef TECTONIC_SHADOWMAPSHADER_H
#define TECTONIC_SHADOWMAPSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

class ShadowMapShader : public Shader {
public:
    ShadowMapShader() = default;
    void init() override;
    void setWVP(const glm::mat4x4& wvp) const;
private:
    GLint loc_wvp = -1;
};

#endif //TECTONIC_SHADOWMAPSHADER_H
