#include "model/texture/CubemapTexture.h"

Logger CubemapTexture::m_logger = Logger("Cubemap Texture");

CubemapTexture::CubemapTexture(const std::array<std::string, CUBEMAP_SIDE_COUNT>& filenames) {
    m_filenames = filenames;

    glGenTextures(1, &m_texObj);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texObj);

    for(uint32_t i = 0; i < ARRAY_SIZE(CUBEMAP_SIDES); i++){
        int width, height;
        void* iData = nullptr;
        int bpp;

        u_char* imageData = stbi_load(m_filenames.at(i).c_str(), &width, &height, &bpp, 0);
        if(!imageData){
            m_logger(Logger::ERROR) << "Couldn't load cubemap side texture " << m_filenames.at(i) << '\n';
            throw textureException("Cannot load ", m_filenames.at(i));
        }

        iData = imageData;

        glTexImage2D(CUBEMAP_SIDES[i], 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, iData);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        stbi_image_free(imageData);

        m_logger(Logger::INFO) << "Loaded cubemap side texture " << filenames.at(i) << '\n';
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void CubemapTexture::bind(GLenum texUnit) const {
    glActiveTexture(texUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texObj);
}

void CubemapTexture::unbind(GLenum texUnit) const {
    glActiveTexture(texUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

CubemapTexture::~CubemapTexture() {
    clean();
}

void CubemapTexture::clean() {
    if(m_texObj != -1)
        glDeleteTextures(1, &m_texObj);
}
