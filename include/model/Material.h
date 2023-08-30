#ifndef TECTONIC_MATERIAL_H
#define TECTONIC_MATERIAL_H

#include <assimp/scene.h>
#include <glm/vec3.hpp>
#include <memory>

#include "utils.h"
#include "model/Texture.h"
#include "defs/TextureDefs.h"

/**
 * Class with information about material on a texture.
 * Contains textures for diffusion and specular light information.
 */
class Material {
public:
    Material() = default;
    Material(const Material&) = default;
    Material(Material&&) = default;
    Material(const aiMaterial* material, const aiScene* scene, const std::string& modelDirectory);

    void bindTextures() const;
    void unbindTextures() const;

    glm::vec3 m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 m_specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float m_shininess = 0.0f;

    std::shared_ptr<Texture> m_diffuseTexture = nullptr;
    std::shared_ptr<Texture> m_specularTexture = nullptr;
    std::shared_ptr<Texture> m_normalTexture = nullptr;

    std::string m_name{};

private:
    void loadName(const aiMaterial* material);
    void loadColors(const aiMaterial* material);
    static std::shared_ptr<Texture> loadTexture(const aiMaterial* material,
                                                const aiScene* scene,
                                                const std::string& modelDirectory,
                                                aiTextureType type);
};

#endif //TECTONIC_MATERIAL_H
