
#include <iostream>
#include "extern/glad/glad.h"
#include "model/Texture.h"
#include "extern/stb_image.h"

std::unordered_map<std::string, std::shared_ptr<Texture>> Texture::m_loadedTextures;
Logger Texture::m_logger = Logger("Texture");

Texture::Texture(GLenum tex_target, const std::string& fileName)
: m_texTarget(tex_target), m_fileName(fileName){
    int width = 0, height = 0, bpp = 0;
    u_char* image_data = stbi_load(m_fileName.c_str(), &width, &height, &bpp, 0);
    if(!image_data){
        m_logger(Logger::ERROR) << "Unable to load texture [" << m_fileName << "]" << '\n';
        throw textureException();
    }
    m_logger(Logger::INFO) << "Loaded texture [" << m_fileName << "]" << '\n';
    loadData(image_data, width, height, bpp);
    stbi_image_free(image_data);
}

Texture::Texture(GLenum tex_target, u_char *data, int32_t length, uint8_t channels, const std::string& fileName)
: m_texTarget(tex_target), m_fileName(fileName){

    int x=0,y=0,bpp=0;
    u_char* image_data = stbi_load_from_memory(data, length, &x, &y, &bpp, channels);
    if(!image_data){
        m_logger(Logger::ERROR) << "Unable to load texture [" << m_fileName << "] from memory" << '\n';
        throw textureException();
    }
    if(!fileName.empty()){
        m_logger(Logger::INFO) << "Loaded embedded texture [" << m_fileName << "]" << '\n';
    }else{
        m_logger(Logger::INFO) << "Loaded embedded texture" << '\n';
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
                glTexImage2D(m_texTarget, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                break;
            case 4:
                glTexImage2D(m_texTarget, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                break;
            default:
                m_logger(Logger::ERROR) << "Unable to load texture [" << m_fileName << "] with " << "[" << bpp << "] channels" << '\n';
                throw textureException();
        }
    }else{
        m_logger(Logger::ERROR) << "Unable to load texture [" << m_fileName << "] with texture target " << "[" << m_texTarget << "]" << '\n';
        throw textureException();
    }

    glTexParameterf(m_texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(m_texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_texTarget, GL_TEXTURE_WRAP_S,  GL_REPEAT);
    glTexParameterf(m_texTarget, GL_TEXTURE_WRAP_T,  GL_REPEAT);

    glGenerateMipmap(m_texTarget);

    glBindTexture(m_texTarget, 0);
}

void Texture::bind(GLenum tex_unit) const {
    glActiveTexture(tex_unit);
    glBindTexture(m_texTarget, m_texObject);
}

void Texture::unbind(GLenum tex_unit) const {
    glActiveTexture(tex_unit);
    glBindTexture(m_texTarget, 0);
}

std::shared_ptr<Texture>
Texture::createTexture(GLenum tex_target, u_char *data, int32_t length, uint8_t channels, const std::string &fileName) {
    if(!fileName.empty() && m_loadedTextures.contains(fileName)){
        m_logger(Logger::DEBUG) << "Texture [" << fileName << "] already loaded" << '\n';
        return m_loadedTextures.at(fileName);
    }

    m_logger(Logger::DEBUG) << "Texture [" << fileName << "] wasn't found in pre-loaded textures" << '\n';
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(tex_target, data, length, channels, fileName);
    m_loadedTextures.insert({fileName, texture});
    return texture;
}

std::shared_ptr<Texture> Texture::createTexture(GLenum tex_target, const std::string &fileName) {
    if(m_loadedTextures.contains(fileName)){
        m_logger(Logger::DEBUG) << "Texture [" << fileName << "] already loaded" << '\n';
        return m_loadedTextures.at(fileName);
    }

    m_logger(Logger::DEBUG) << "Texture [" << fileName << "] wasn't found in pre-loaded textures" << '\n';
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(tex_target, fileName);
    m_loadedTextures.insert({fileName, texture});
    return texture;
}

const std::string &Texture::name() {
    return m_fileName;
}
