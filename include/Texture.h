#ifndef TECTONIC_TEXTURE_H
#define TECTONIC_TEXTURE_H

#include "exceptions.h"
#include <string>

class Texture {
public:
    Texture(GLenum tex_target, std::string  file_name);

    void load();
    void bind(GLenum tex_unit) const;

private:
    std::string _file_name;
    GLenum _tex_target;
    GLuint _tex_object;
};

#endif //TECTONIC_TEXTURE_H
