#include <memory>

#include "model/Material.h"

Material::Material(const aiMaterial *material, const aiScene* scene, const std::string& modelDirectory) {
    loadName(material);
    loadColors(material);
    m_diffuseTexture = loadTexture(material, scene, modelDirectory, aiTextureType_DIFFUSE);
    m_specularTexture = loadTexture(material, scene, modelDirectory, aiTextureType_SPECULAR);
    m_normalTexture = loadTexture(material, scene, modelDirectory, aiTextureType_NORMALS);
}

void Material::loadName(const aiMaterial *material) {
    aiString name;
    if(material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        m_name = name.C_Str();
}

std::shared_ptr<Texture> Material::loadTexture(const aiMaterial *material, const aiScene* scene, const std::string& modelDirectory, aiTextureType type) {
    if(material->GetTextureCount(type) > 0){
        aiString path;
        if(material->GetTexture(type, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);
            const aiTexture* texture;
            if((texture = scene->GetEmbeddedTexture(path.C_Str()))){
                std::string hint = texture->achFormatHint;
                if(hint == "png" || hint == "jpg"){
                    if(texture->mHeight == 0){
                        return std::make_shared<Texture>(GL_TEXTURE_2D, (u_char*)texture->pcData, texture->mWidth, 0);
                    }
                }
            }else{
                if (p.substr(0, 2) == ".\\") {
                    p = p.substr(2, p.size() - 2);
                }
                std::string fullpath;
                fullpath.append(modelDirectory).append("/").append(p);
                return std::make_shared<Texture>(GL_TEXTURE_2D, fullpath);
            }
        }
    }
    return nullptr;
}

void Material::loadColors(const aiMaterial *material) {
    aiColor3D ambient_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_AMBIENT, ambient_color) == AI_SUCCESS){
        m_ambientColor = Utils::aiColToGLM(ambient_color);
    }

    aiColor3D diffuse_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == AI_SUCCESS){
        m_diffuseColor = Utils::aiColToGLM(diffuse_color);
    }

    aiColor3D specular_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == AI_SUCCESS){
        m_specularColor = Utils::aiColToGLM(specular_color);
    }

    float shininess = 0.0f;
    if(material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS){
        m_shininess = shininess;
    }
}

void Material::bindTextures() const {
    if(m_diffuseTexture)
        m_diffuseTexture->bind(COLOR_TEXTURE_UNIT);
    if(m_specularTexture)
        m_specularTexture->bind(SPECULAR_EXPONENT_UNIT);
    if(m_normalTexture)
        m_normalTexture->bind(NORMAL_TEXTURE_UNIT);
}

void Material::unbindTextures() const {
    if(m_diffuseTexture)
        m_diffuseTexture->unbind(COLOR_TEXTURE_UNIT);
    if(m_specularTexture)
        m_specularTexture->unbind(SPECULAR_EXPONENT_UNIT);
    if(m_normalTexture)
        m_normalTexture->unbind(NORMAL_TEXTURE_UNIT);
}
