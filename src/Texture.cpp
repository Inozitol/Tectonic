
#include <iostream>
#include "extern/glad/glad.h"
#include "model/texture/Texture.h"
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
    glCreateTextures(m_texTarget, 1, &m_texObject);
    //glGenTextures(1, &m_texObject);
    //glBindTexture(m_texTarget, m_texObject);
    if(m_texTarget == GL_TEXTURE_2D){
        switch(bpp){
            case 1:
                glTextureStorage2D(m_texObject, 1, GL_R8, width, height);
                glTextureSubImage2D(m_texObject, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data);
                break;
            case 3:
                glTextureStorage2D(m_texObject, 1, GL_RGB8, width, height);
                glTextureSubImage2D(m_texObject, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
                break;
            case 4:
                glTextureStorage2D(m_texObject, 1, GL_RGBA8, width, height);
                glTextureSubImage2D(m_texObject, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
                break;
            default:
                m_logger(Logger::ERROR) << "Unable to load texture [" << m_fileName << "] with " << "[" << bpp << "] channels" << '\n';
                throw textureException();
        }
    }else{
        m_logger(Logger::ERROR) << "Unable to load texture [" << m_fileName << "] with texture target " << "[" << m_texTarget << "]" << '\n';
        throw textureException();
    }

    glTextureParameterf(m_texObject, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameterf(m_texObject, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameterf(m_texObject, GL_TEXTURE_WRAP_S,  GL_REPEAT);
    glTextureParameterf(m_texObject, GL_TEXTURE_WRAP_T,  GL_REPEAT);

    glGenerateTextureMipmap(m_texObject);

    m_bindlessHandle = glGetTextureHandleARB(m_texObject);
    if(m_bindlessHandle == 0){
        m_logger(Logger::ERROR) << "Unable to retrieve texture handle for texture [" << m_fileName << "]" << '\n';
        throw textureException();
    }

    glMakeTextureHandleResidentARB(m_bindlessHandle);
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

GLuint64 Texture::getHandle() const {
    return m_bindlessHandle;
}
