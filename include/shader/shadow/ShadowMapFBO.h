#ifndef TECTONIC_SHADOWMAPFBO_H
#define TECTONIC_SHADOWMAPFBO_H

#include <cstdint>
#include "extern/glad/glad.h"
#include "exceptions.h"


class ShadowMapFBO {
public:
    ShadowMapFBO() = default;
    ~ShadowMapFBO();
    void init(int32_t width, int32_t height);
    void clean();
    void bind4writing() const;
    void bind4reading(GLenum tex_unit) const;

private:
    int32_t m_width = 0;
    int32_t m_height = 0;
    GLuint m_fbo = -1;
    GLuint m_shadowMap = -1;
};


#endif //TECTONIC_SHADOWMAPFBO_H
