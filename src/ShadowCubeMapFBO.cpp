#include "shader/shadow/ShadowCubeMapFBO.h"


ShadowCubeMapFBO::~ShadowCubeMapFBO() {
    clean();
}

void ShadowCubeMapFBO::init(int32_t size) {
    m_size = size;

    // Create depth buffer
    glGenTextures(1, &m_depth);
    glBindTexture(GL_TEXTURE_2D, m_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_size, m_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create cube map
    glGenTextures(1, &m_shadowCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_shadowCubeMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for(uint32_t i = 0; i < 6; i++){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F, m_size, m_size, 0, GL_RED, GL_FLOAT, nullptr);
    }

    // Create FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE){
        throw shadowMapException("Unable to create framebuffer for cube map");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void ShadowCubeMapFBO::clean(){
    if(m_fbo != -1){
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = -1;
    }
    if(m_shadowCubeMap != -1){
        glDeleteTextures(1, &m_shadowCubeMap);
        m_shadowCubeMap = -1;
    }
    if(m_depth != -1){
        glDeleteTextures(1, &m_depth);
        m_depth = -1;
    }
}

void ShadowCubeMapFBO::bind4writing(GLenum cubeFace) const {
    glViewport(0, 0, m_size, m_size);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeFace, m_shadowCubeMap, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void ShadowCubeMapFBO::bind4reading(GLenum texUnit) const {
    glActiveTexture(texUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_shadowCubeMap);
}
