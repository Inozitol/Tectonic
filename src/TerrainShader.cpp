#include "shader/TerrainShader.h"

void TerrainShader::init() {
    Shader::init();

    addShader(GL_VERTEX_SHADER, TERRAIN_VERT_SHADER_PATH);
    addShader(GL_FRAGMENT_SHADER, TERRAIN_FRAG_SHADER_PATH);
    finalize();

    loc_WVP = cacheUniform("u_WVP");
    loc_minHeight = cacheUniform("u_minHeight");
    loc_maxHeight = cacheUniform("u_maxHeight");

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_sampler.height); i++) {
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_samplers.height[%d]", i);
        loc_sampler.height[i] = cacheUniform(name);
    }

    for(uint32_t i = 0; i < ARRAY_SIZE(loc_textureHeight); i++) {
        char name[128];
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "u_textureHeight[%d]", i);
        loc_textureHeight[i] = cacheUniform(name);
    }

    loc_numTextureHeights = cacheUniform("u_textureHeightCount");

    loc_dirLight.color = cacheUniform("u_directionalLight.base.color");
    loc_dirLight.ambientIntensity = cacheUniform("u_directionalLight.base.ambientIntensity");
    loc_dirLight.diffuseIntensity = cacheUniform("u_directionalLight.base.diffuseIntensity");
    loc_dirLight.direction = cacheUniform("u_directionalLight.direction");
}

void TerrainShader::setWVP(const glm::mat4 &wvp) const {
    glUniformMatrix4fv(getUniformLocation(loc_WVP), 1, GL_FALSE, glm::value_ptr(wvp));
}

void TerrainShader::setMinHeight(float minHeight) const {
    glUniform1f(getUniformLocation(loc_minHeight), minHeight);
}

void TerrainShader::setMaxHeight(float maxHeight) const {
    glUniform1f(getUniformLocation(loc_maxHeight), maxHeight);
}

void TerrainShader::setBlendedTextures(const std::vector<float>& heights, uint32_t textureCount) {
    glUniform1ui(getUniformLocation(loc_numTextureHeights), textureCount);
    for(uint32_t i = 0; i < textureCount; i++){
        glUniform1f(getUniformLocation(loc_textureHeight[i]), heights.at(i));
    }
}

void TerrainShader::setBlendedTextureSamples(GLint texUnit) const {
    for(uint32_t i = 0; i < MAX_TERRAIN_HEIGHT_TEXTURE; i++) {
        glUniform1i(getUniformLocation(loc_sampler.height[i]), texUnit+i);
    }
}

void TerrainShader::setDirectionalLight(const DirectionalLight &light) const {
    const glm::vec3 direction = light.getDirection();
    glUniform3f(getUniformLocation(loc_dirLight.color), light.color.r, light.color.g, light.color.b);
    glUniform1f(getUniformLocation(loc_dirLight.ambientIntensity), light.ambientIntensity);
    glUniform1f(getUniformLocation(loc_dirLight.diffuseIntensity), light.diffuseIntensity);
    glUniform3f(getUniformLocation(loc_dirLight.direction), direction.x, direction.y, direction.z);

}
