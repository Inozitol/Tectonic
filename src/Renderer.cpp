#include "Renderer.h"

void Renderer::queueRender(const ObjectData &object, Model* model) {
    GLuint vao = model->getVAO();
    if(!m_drawQueue.contains(vao)) {
        m_drawQueue.insert({vao, meshQueue_t()});
        m_drawQueue.at(vao).resize(model->m_materials.size());
    }
    for(const auto& mesh: model->m_meshes){
        m_drawQueue.at(vao).at(mesh.matIndex).first = model->getMaterial(mesh.matIndex);
        m_drawQueue.at(vao).at(mesh.matIndex).second.push_back(Drawable{object, model, &mesh});
    }
}

void Renderer::renderQueue() {
    m_gameCamera->createView();

    clearRender();

    if(m_cursorPressed) {
        m_pickingShader.enable();
        m_pickingTexture.enableWriting();
        glViewport(0,0, m_windowWidth, m_windowHeight);

        glCullFace(GL_BACK);

        for (auto &[vao, queue]: m_drawQueue) {
            glBindVertexArray(vao);
            pickingPass(queue);
        }

        m_pickingTexture.disableWriting();
    }

    /*
    for(auto& [vao, queue]: m_drawQueue){
        glBindVertexArray(vao);
        shadowPass(queue);
    }
    */

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,m_windowWidth,m_windowHeight);
    m_lightingShader.enable();

    glCullFace(GL_BACK);

    m_shadowMapFBO.bind4reading(SHADOW_TEXTURE_UNIT);
    m_dirLight->createView();

    for (auto &[vao, queue]: m_drawQueue) {
        glBindVertexArray(vao);
        lightingPass(queue);
    }

    if(m_debugEnabled) {
        m_debugShader.enable();
        glViewport(0,0,m_windowWidth, m_windowHeight);

        glCullFace(GL_BACK);

        for (auto &[vao, queue]: m_drawQueue) {
            glBindVertexArray(vao);
            debugPass(queue);
        }
    }

    glBindVertexArray(0);

    if(m_cursorPressed){
        const auto& pixel = m_pickingTexture.readPixel(m_cursorPosX, m_windowHeight-m_cursorPosY-1);

        if(pixel.objectIndex != 0){
            sig_objectClicked.emit(pixel.objectIndex-1);
        }

        m_cursorPressed = false;
    }

    m_drawQueue.clear();
}

void Renderer::clearRender() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,m_windowWidth,m_windowHeight);
    //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClearColor(0.027, 0.769, 0.702, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_shadowMapFBO.bind4writing();
    glClear(GL_DEPTH_BUFFER_BIT);

    m_pickingShader.enable();
    m_pickingTexture.enableWriting();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(0);
}

void Renderer::shadowPass(const meshQueue_t &queue) {
    m_shadowMapFBO.bind4writing();
    m_shadowMapShader.enable();

    glCullFace(GL_FRONT);

    m_dirLight->updateTightOrthoProjection(*m_gameCamera);
    m_gameCamera->setOrthographicInfo(m_dirLight->shadowOrthoInfo);
    m_dirLight->createView();

    for(const auto & matVector : queue){
        for(const auto& drawable : matVector.second) {
            renderModelShadow(drawable);
        }
    }
}

void Renderer::renderModelShadow(const Drawable& drawable) {
    glm::mat4 mashMatrix = drawable.object.transformation.getMatrix();

    glm::mat4 wvp = m_spotLights->at(0).getWVP(mashMatrix);
    m_shadowMapShader.setWVP(wvp);
    m_shadowMapShader.setWorld(mashMatrix);

    renderMesh(*drawable.mesh);
}

void Renderer::lightingPass(const meshQueue_t &queue) {

    for(const auto & matVector : queue){
        const Material* material = matVector.first;

        if(material){
            m_lightingShader.setMaterial(*material);
            material->bindTextures();
        }

        for(const auto& drawable : matVector.second) {
            renderModelLight(drawable);
        }

        if(material) {
            material->unbindTextures();
        }
    }
}

void Renderer::renderModelLight(const Drawable& drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_lightingShader.setWVP(wvp);
    m_lightingShader.setWorld(objectTransform);

    // Light point of view
    glm::mat4 light_wvp = m_dirLight->getWVP(objectTransform);
    m_lightingShader.setLightWVP(light_wvp);

    // Setup dir light
    m_lightingShader.setDirectionalLight(*m_dirLight);

    m_lightingShader.setWorldCameraPos(m_gameCamera->getPosition());

    m_lightingShader.setSpotLights(m_spotLightsCount, *m_spotLights);
    m_lightingShader.setPointLights(m_pointLightsCount, *m_pointLights);

    m_lightingShader.setColorMod(drawable.object.colorMod);

    renderMesh(*drawable.mesh);
}

inline void Renderer::renderMesh(const Mesh &mesh) {
    glDrawElementsBaseVertex(GL_TRIANGLES,
                             static_cast<GLsizei>(mesh.indicesCount),
                             GL_UNSIGNED_INT,
                             (void *)(mesh.indicesOffset * sizeof(uint32_t)),
                             mesh.verticesOffset);
}

void Renderer::pickingPass(const meshQueue_t &queue) {

    for(const auto & matVector : queue){
        for(const auto& drawable : matVector.second) {
            renderModelPicking(drawable);
        }
    }

}

void Renderer::renderModelPicking(const Drawable& drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_pickingShader.setWVP(wvp);
    m_pickingShader.setObjectIndex(drawable.object.index+1);

    renderMesh(*drawable.mesh);
}

void Renderer::debugPass(const Renderer::meshQueue_t &queue) {

    for(const auto & matVector : queue){
        for(const auto& drawable : matVector.second) {
            renderModelDebug(drawable);
        }
    }
}

void Renderer::renderModelDebug(const Drawable &drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_debugShader.setWVP(wvp);
    m_debugShader.setWorld(objectTransform);

    renderMesh(*drawable.mesh);
}

void Renderer::setWindowSize(int32_t width, int32_t height) {
    m_windowWidth = width;
    m_windowHeight = height;

    // Need to change picking texture dimensions
    m_pickingTexture.init(m_windowWidth, m_windowHeight);
}

Renderer::Renderer() {
    try {
        initGLFW();
        initGL();
        initShaders();
    }catch(tectonicException& e){
        fprintf(stderr, "EXCEPTION: %s", e.what());
        exit(-1);
    }
}

Renderer::~Renderer() {
    window.reset();
    glfwTerminate();
}

void Renderer::initGLFW() {
    glfwSetErrorCallback(Renderer::glfwErrorCallback);

    if (!glfwInit())
        throw rendererException("Renderer couldn't init GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    window = std::make_shared<Window>("Howdy World");

    window->makeCurrentContext();
}

void Renderer::initGL() {
    gladLoadGL();
    glfwSwapInterval(0);

    // Enable culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable error callback
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(Renderer::openGLErrorCallback, nullptr);
}

void Renderer::initShaders() {
    m_lightingShader.init();

    m_lightingShader.enable();
    m_lightingShader.setDiffuseTextureUnit(COLOR_TEXTURE_UNIT_INDEX);
    m_lightingShader.setSpecularTextureUnit(SPECULAR_EXPONENT_UNIT_INDEX);
    m_lightingShader.setNormalTextureUnit(NORMAL_TEXTURE_UNIT_INDEX);
    m_lightingShader.setShadowMapTextureUnit(SHADOW_TEXTURE_UNIT_INDEX);
    m_lightingShader.setShadowCubeMapTextureUnit(SHADOW_CUBE_MAP_TEXTURE_UNIT_INDEX);

    m_shadowMapShader.init();

    m_shadowMapFBO.init(SHADOW_WIDTH, SHADOW_HEIGHT);

    m_shadowCubeMapFBO.init(1000);

    m_pickingShader.init();
    m_pickingTexture.init(m_windowWidth, m_windowHeight);

    m_debugShader.init();
}

void Renderer::glfwErrorCallback(int, const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

void Renderer::openGLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                   const GLchar *message, const void *userParam) {
    switch(severity){
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        case GL_DEBUG_SEVERITY_LOW:
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            fprintf( stderr, "GL CALLBACK:%s type = 0x%x, severity = MEDIUM, message = %s\n",
                     ( type == GL_DEBUG_TYPE_ERROR ? " ** GL ERROR **" : "" ),
                     type, message );
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            fprintf( stderr, "GL CALLBACK:%s type = 0x%x, severity = HIGH, message = %s\n",
                     ( type == GL_DEBUG_TYPE_ERROR ? " ** GL ERROR **" : "" ),
                     type, message );
            break;
        default:
            fprintf( stderr, "GL CALLBACK:%s type = 0x%x, severity = UNKNOWN, message = %s\n",
                     ( type == GL_DEBUG_TYPE_ERROR ? " ** GL ERROR **" : "" ),
                     type, message );
            break;
    }
}



