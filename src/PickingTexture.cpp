#include "PickingTexture.h"

PickingTexture::~PickingTexture() {
    clean();
}

void PickingTexture::init(int32_t winWidth, int32_t winHeight) {
    if(winWidth != 0 && winHeight != 0) {
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        initTextures(winWidth, winHeight);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            throw tectonicException("Incorrect picking texture init");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingTexture::clean() {
    if(m_fbo != -1) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = -1;
    }

    if(m_pickingTexture != -1) {
        glDeleteTextures(1, &m_pickingTexture);
        m_pickingTexture = -1;
    }

    if(m_depthTexture != -1) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = -1;
    }
}

void PickingTexture::enableWriting() const {
    glViewport(0, 0, m_width, m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void PickingTexture::disableWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PickingTexture::pixelInfo PickingTexture::readPixel(int32_t x, int32_t y) const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    pixelInfo pixel;
    glReadPixels(x, y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_SHORT, &pixel);

    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return pixel;
}

void PickingTexture::initTextures(int32_t winWidth, int32_t winHeight) {
    m_width = winWidth;
    m_height = winHeight;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    if(m_pickingTexture == -1)
        glDeleteTextures(1, &m_pickingTexture);

    glGenTextures(1, &m_pickingTexture);
    glBindTexture(GL_TEXTURE_2D, m_pickingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16UI, m_width, m_height, 0, GL_RG_INTEGER, GL_UNSIGNED_SHORT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pickingTexture, 0);

    if(m_depthTexture != -1)
        glDeleteTextures(1, &m_depthTexture);

    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

}
