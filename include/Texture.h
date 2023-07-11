#ifndef TECTONIC_TEXTURE_H
#define TECTONIC_TEXTURE_H

#include "exceptions.h"
#include <string>

class Texture {
public:
    Texture(GLenum tex_target, std::string  file_name);
    Texture(GLenum tex_target, u_char *data, uint32_t length, uint8_t channels);

    void load();
    void bind(GLenum tex_unit) const;

private:
    void load_data(u_char* data, int width, int height, int bpp);

    std::string _file_name;
    GLenum _tex_target;
    GLuint _tex_object;
};

#endif //TECTONIC_TEXTURE_H
