#ifndef TECTONIC_SKYBOXSHADER_H
#define TECTONIC_SKYBOXSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "defs/ConfigDefs.h"

class SkyboxShader : public Shader {
public:
    SkyboxShader() : Shader(ShaderType::BASIC_SHADER){}
    void init() override;

    void setVP(const glm::mat4& vp) const;
    void setCubemapUnit(GLint texUnit);

private:
    uint32_t loc_VP = -1;
    struct{
        uint32_t cubemap = -1;
    } loc_sampler;
};


#endif //TECTONIC_SKYBOXSHADER_H
