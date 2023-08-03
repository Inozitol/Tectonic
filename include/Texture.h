#ifndef TECTONIC_TEXTURE_H
#define TECTONIC_TEXTURE_H

#include "exceptions.h"
#include <string>

/**
 * Class representing a texture. Stores texture data inside OpenGL buffers.
 */
class Texture {
public:
    Texture(GLenum tex_target, std::string  file_name);
    Texture(GLenum tex_target, u_char *data, int32_t length, uint8_t channels);

    void bind(GLenum tex_unit) const;
    void unbind(GLenum tex_unit) const;

private:
    void loadData(u_char* data, int32_t width, int32_t height, uint8_t bpp);

    std::string m_fileName;
    GLenum m_texTarget = -1;
    GLuint m_texObject = -1;
};

#endif //TECTONIC_TEXTURE_H
