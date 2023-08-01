#include "shader/shadow/ShadowMapFBO.h"

ShadowMapFBO::~ShadowMapFBO() {
    if(m_fbo != -1){
        glDeleteFramebuffers(1, &m_fbo);
    }
    if(m_shadowMap != -1){
        glDeleteTextures(1, &m_shadowMap);
    }
}

void ShadowMapFBO::init(int32_t width, int32_t height) {
    m_width = width;
    m_height = height;

    // Create FBO
    glGenFramebuffers(1, &m_fbo);

    // Create depth buffer
    glGenTextures(1, &m_shadowMap);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if(status != GL_FRAMEBUFFER_COMPLETE){
        throw shadowMapException("FBO error");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapFBO::bind4writing() const {
    glViewport(0, 0, m_width, m_height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
}

void ShadowMapFBO::bind4reading(GLenum tex_unit) const {
    glActiveTexture(tex_unit);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
}
