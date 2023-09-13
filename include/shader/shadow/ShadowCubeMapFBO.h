#ifndef TECTONIC_SHADOWCUBEMAPFBO_H
#define TECTONIC_SHADOWCUBEMAPFBO_H

#include <cstdint>
#include "extern/glad/glad.h"
#include "exceptions.h"

class ShadowCubeMapFBO {
public:
    ShadowCubeMapFBO() = default;
    ~ShadowCubeMapFBO();

    void clean();

    void init(int32_t size);
    void bind4writing(GLenum cubeFace) const;
    void bind4reading(GLenum texUnit) const;

private:
    int32_t m_size = 0;
    GLuint m_fbo = -1;
    GLuint m_shadowCubeMap = -1;
    GLuint m_depth = -1;
};

#endif //TECTONIC_SHADOWCUBEMAPFBO_H
