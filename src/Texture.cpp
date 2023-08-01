
#include <iostream>
#include <utility>
#include <GL/gl.h>
#include "Texture.h"
#include "extern/stb_image.h"

Texture::Texture(GLenum tex_target, std::string  file_name)
: m_texTarget(tex_target), m_fileName(std::move(file_name)){
    int width = 0, height = 0, bpp = 0;
    u_char* image_data = stbi_load(m_fileName.c_str(), &width, &height, &bpp, 0);
    if(!image_data){
        throw textureException("Unable to load texture [", m_fileName, "]");
    }
    std::cout << "Loaded texture [" << m_fileName << "]" << std::endl;
    loadData(image_data, width, height, bpp);
    stbi_image_free(image_data);
}

Texture::Texture(GLenum tex_target, u_char *data, int32_t length, uint8_t channels) : m_texTarget(tex_target) {
    int x=0,y=0,bpp=0;
    u_char* image_data = stbi_load_from_memory(data, length, &x, &y, &bpp, channels);
    if(!image_data){
        throw textureException("Unable to load texture from memory");
    }
    loadData(image_data, x, y, bpp);
    stbi_image_free(image_data);
}

void Texture::loadData(u_char* data, int32_t width, int32_t height, uint8_t bpp) {
    glGenTextures(1, &m_texObject);
    glBindTexture(m_texTarget, m_texObject);
    if(m_texTarget == GL_TEXTURE_2D){
        switch(bpp){
            case 1:
                glTexImage2D(m_texTarget, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
                break;
            case 3:
            case 4:
                glTexImage2D(m_texTarget, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                break;
            default:
                throw textureException("Texture type not implemented");
        }
    }else{
        throw textureException("Support for texture target is not implemented");
    }

    glTexParameterf(m_texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(m_texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_texTarget, GL_TEXTURE_WRAP_S,  GL_REPEAT);
    glTexParameterf(m_texTarget, GL_TEXTURE_WRAP_T,  GL_REPEAT);

    glBindTexture(m_texTarget, 0);
}

void Texture::bind(GLenum tex_unit) const {
    glActiveTexture(tex_unit);
    glBindTexture(m_texTarget, m_texObject);
}

