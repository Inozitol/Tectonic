
#ifndef TECTONIC_TERRAINSHADER_H
#define TECTONIC_TERRAINSHADER_H

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "defs/ConfigDefs.h"
#include "model/Material.h"
#include "model/terrain/Terrain.h"
#include "shader/LightingShader.h"

class TerrainShader : public Shader {
public:
    TerrainShader() : Shader(ShaderType::BASIC_SHADER){}
    void init() override;

    void setWVP(const glm::mat4& wvp) const;
    void setMinHeight(float minHeight) const;
    void setMaxHeight(float maxHeight) const;
    void setBlendedTextures(const std::array<std::pair<float, std::shared_ptr<Texture>>, MAX_TERRAIN_HEIGHT_TEXTURE>& heights, uint32_t textureCount);
    void setDirectionalLight(const DirectionalLight &light) const;

private:
    uint32_t loc_WVP = -1;
    uint32_t loc_minHeight = -1;
    uint32_t loc_maxHeight = -1;

    struct {
        uint32_t height[MAX_TERRAIN_HEIGHT_TEXTURE]{};
    } loc_sampler;

    uint32_t loc_textureHeight[MAX_TERRAIN_HEIGHT_TEXTURE]{};
    uint32_t loc_numTextureHeights = -1;

    // Definition of a directional light
    struct {
        uint32_t color = -1;
        uint32_t ambientIntensity = -1;
        uint32_t diffuseIntensity = -1;
        uint32_t direction = -1;
    } loc_dirLight;

};

#endif //TECTONIC_TERRAINSHADER_H
