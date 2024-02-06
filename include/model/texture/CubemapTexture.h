#ifndef TECTONIC_CUBEMAPTEXTURE_H
#define TECTONIC_CUBEMAPTEXTURE_H

#include <string>
#include <array>

#include "utils.h"
#include "Logger.h"
#include "extern/stb/stb_image.h"
#include "exceptions.h"

#define CUBEMAP_SIDE_COUNT 6

static const GLenum CUBEMAP_SIDES[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

class CubemapTexture {
public:
    explicit CubemapTexture(const std::array<std::string, CUBEMAP_SIDE_COUNT>& filenames);
    ~CubemapTexture();

    void bind(GLenum texUnit) const;
    void unbind(GLenum texUnit) const;
    void clean();

private:
    std::array<std::string, CUBEMAP_SIDE_COUNT> m_filenames;
    GLuint m_texObj = -1;

    static Logger m_logger;
};


#endif //TECTONIC_CUBEMAPTEXTURE_H
