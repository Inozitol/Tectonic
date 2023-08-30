#ifndef TECTONIC_PICKINGTEXTURE_H
#define TECTONIC_PICKINGTEXTURE_H

#include "extern/glad/glad.h"
#include "exceptions.h"

class PickingTexture {
public:
    PickingTexture() = default;
    ~PickingTexture();

    void init(int32_t winWidth, int32_t winHeight);
    void initTextures(int32_t winWidth, int32_t winHeight);
    void enableWriting() const;
    void disableWriting() const;

    struct pixelInfo{
        uint32_t objectIndex = 0;
        uint32_t drawIndex = 0;
        uint32_t primIndex = 0;
    };

    pixelInfo readPixel(int32_t x, int32_t y);
private:

    int32_t m_width = 0, m_height = 0;

    GLuint m_fbo = -1;
    GLuint m_pickingTexture = -1;
    GLuint m_depthTexture = -1;
};


#endif //TECTONIC_PICKINGTEXTURE_H
