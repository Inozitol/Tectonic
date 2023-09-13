#include "model/Material.h"

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
