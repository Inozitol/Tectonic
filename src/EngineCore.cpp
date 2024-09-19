#include "engine/EngineCore.h"

#include "engine/TecCache.h"

void EngineCore::run() {
    m_vktCore.setProjMatrix(m_gameCamera->getProjectionMatrix());

    double prevTime = glfwGetTime();
    double currTime = prevTime;
    TecCache::deltaTime = currTime-prevTime;
    uint32_t ctr = 0;
    // TODO
    //  Seems like a bad idea to use shouldClose().
    //  The m_window isn't a hamster in a wheel.
    //  Should get close bool with a signal from Window class.
    while(!m_vktCore.shouldClose()) {
        ctr++;
        prevTime = currTime;
        currTime = glfwGetTime();
        TecCache::deltaTime = (currTime - prevTime);

        if(ctr % 200 < 50) {
            bobAnimatrix.actions[0].target = {1.0f, 5.0f, 0.0f};
            bobAnimatrix.actions[1].target = {3.0f, 6.0f, 0.0f};
            bobAnimatrix.actions[2].target = {-3.0f,3.0f, -6.0f};
            //bobAnimatrix.actions[3].target = {0.0f, -2.0f, 0.0f};
        }else if (ctr % 200 < 100){
            bobAnimatrix.actions[0].target = {1.0f, 4.0f, 0.0f};
            bobAnimatrix.actions[1].target = {3.0f, 0.0f, 0.0f};
            bobAnimatrix.actions[2].target = {-3.0f, 3.0f, 6.0f};
            //bobAnimatrix.actions[3].target = {0.0f, -2.0f, -5.0f};
        }else if (ctr % 200 < 150){
            bobAnimatrix.actions[0].target = {-1.0f, 5.0f, 0.0f};
            bobAnimatrix.actions[1].target = {3.0f, 6.0f, 0.0f};
            bobAnimatrix.actions[2].target = {-3.0f, 3.0f, 6.0f};
            //bobAnimatrix.actions[3].target = {0.0f, -2.0f, 0.0f};
        }else{
            bobAnimatrix.actions[0].target = {-1.0f, 4.0f, 0.0f};
            bobAnimatrix.actions[1].target = {3.0f, 0.0f, 0.0f};
            bobAnimatrix.actions[2].target = {-3.0f, 3.0f, -6.0f};
            //bobAnimatrix.actions[3].target = {0.0f, -2.0f, 5.0f};
        }
        bobAnimatrix.updateActions();

        //m_objects[0]->model.transformation.setRotation(0.0f,glfwGetTime()*50.0f, 0.0f);
        m_vktCore.cameraPosition = m_gameCamera->getPosition();
        m_vktCore.cameraDirection = m_gameCamera->getDirection();
        glfwPollEvents();
        m_gameCamera->createView();
        m_gameCamera->updatePosition();
        m_vktCore.setViewMatrix(m_gameCamera->getViewMatrix());
        m_vktCore.run();
    }
}

void EngineCore::clean() {
    if(m_isInitialized) {
        m_vktCore.clear();
        m_window.reset();
        glfwTerminate();

        m_isInitialized = false;
    }
}

/*
void EngineCore::queueModelRender(const ObjectData &object, Model* model) {
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

void EngineCore::queueSkinnedModelRender(const SkinnedObjectData &object, SkinnedModel *skinnedModel) {
    GLuint vao = skinnedModel->getVAO();
    if(!m_skinnedDrawQueue.contains(vao)){
        m_skinnedDrawQueue.insert({vao, skinnedMeshQueue_t()});
        m_skinnedDrawQueue.at(vao).resize(skinnedModel->m_materials.size());
    }
    for(const auto& mesh: skinnedModel->m_meshes){
        m_skinnedDrawQueue.at(vao).at(mesh.matIndex).first.first = skinnedModel->getMaterial(mesh.matIndex);
        m_skinnedDrawQueue.at(vao).at(mesh.matIndex).first.second = object.animator.getFinalBoneMatrices();
        m_skinnedDrawQueue.at(vao).at(mesh.matIndex).second.push_back(SkinnedDrawable{object, skinnedModel, &mesh});
    }
}

void EngineCore::setTerrainModelRender(const std::shared_ptr<Terrain>& terrain) {
    m_terrain =  terrain;
    m_terrainShader.enable();
    float min,max;
    std::tie(min,max) = m_terrain->getMinMaxHeight();
    m_terrainShader.setMinHeight(min);
    m_terrainShader.setMaxHeight(max);

    m_terrainShader.setBlendedTextures(m_terrain->m_blendingTextures, m_terrain->m_blendingTexturesCount);
}

void EngineCore::setSkyboxModelRender(const std::shared_ptr<Skybox> &skybox) {
    m_skyboxColor = skybox;
}

void EngineCore::renderQueues() {
    clearRender();

    /// Terrain shader
    if(m_terrain) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_terrainShader.enable();
        glViewport(0,0, m_windowWidth, m_windowHeight);

        glCullFace(GL_BACK);
        glBindVertexArray(m_terrain->getVAO());
        renderTerrain();
    }

    /// Picking phase
    if(m_cursorPressed) {
        m_pickingShader.enable(Shader::ShaderType::BONE_SHADER);
        m_pickingTexture.enableWriting();

        glCullFace(GL_BACK);

        for (auto &[vao, queue]: m_skinnedDrawQueue) {
            glBindVertexArray(vao);
            pickingPass(queue);
        }

        m_pickingShader.enable(Shader::ShaderType::BASIC_SHADER);

        for (auto &[vao, queue]: m_drawQueue) {
            glBindVertexArray(vao);
            pickingPass(queue);
        }

        m_pickingTexture.disableWriting();
    }


    /// Shadow phase

    m_shadowMapFBO.bind4writing();

    m_dirLight->updateTightOrthoProjection(*m_gameCamera);
    m_gameCamera->setOrthographicInfo(m_dirLight->shadowOrthoInfo);
    m_dirLight->createView();

    m_shadowMapShader.enable(Shader::ShaderType::BONE_SHADER);
    m_shadowMapShader.setLightWorldPos(m_spotLights->at(0).getPosition());
    m_spotLights->at(0).createView();

    for(auto& [vao, queue]: m_skinnedDrawQueue){
        glBindVertexArray(vao);
        shadowPass(queue);
    }

    m_shadowMapShader.enable(Shader::ShaderType::BASIC_SHADER);
    m_shadowMapShader.setLightWorldPos(m_spotLights->at(0).getPosition());

    for(auto& [vao, queue]: m_drawQueue){
        glBindVertexArray(vao);
        shadowPass(queue);
    }

    /// Lighting phase

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,m_windowWidth,m_windowHeight);
    glCullFace(GL_BACK);

    m_shadowMapFBO.bind4reading(SHADOW_TEXTURE_UNIT);

    m_lightingShader.enable(Shader::ShaderType::BONE_SHADER);
    for (auto &[vao, queue]: m_skinnedDrawQueue) {
        glBindVertexArray(vao);
        lightingPass(queue);
    }

    m_lightingShader.enable(Shader::ShaderType::BASIC_SHADER);
    for (auto &[vao, queue]: m_drawQueue) {
        glBindVertexArray(vao);
        lightingPass(queue);
    }

    // Skybox phase
    if(m_skyboxColor) {
        glCullFace(GL_FRONT);
        glDepthFunc(GL_LEQUAL);
        m_skyboxShader.enable();
        glBindVertexArray(m_skyboxColor->getVAO());
        renderSkybox();
        glDepthFunc(GL_LESS);
    }


    /// Debug phase

    if(m_debugEnabled) {
        m_debugShader.enable(Shader::ShaderType::BONE_SHADER);
        glViewport(0,0,m_windowWidth, m_windowHeight);

        glCullFace(GL_BACK);

        for (auto &[vao, queue]: m_skinnedDrawQueue) {
            glBindVertexArray(vao);
            debugPass(queue);
        }

        m_debugShader.enable(Shader::ShaderType::BASIC_SHADER);
        for (auto &[vao, queue]: m_drawQueue) {
            glBindVertexArray(vao);
            debugPass(queue);
        }
    }

    glBindVertexArray(0);

    if(m_cursorPressed){
        const auto& pixel = m_pickingTexture.readPixel(m_cursorPosX, m_windowHeight-m_cursorPosY-1);

        if(pixel.objectIndex != 0){
            if(pixel.objectFlags & PickingTexture::SKINNED){
                sig_skinnedObjectClicked.emit(pixel.objectIndex-1);
            }else {
                sig_objectClicked.emit(pixel.objectIndex - 1);
            }
        }

        m_cursorPressed = false;
    }

    m_drawQueue.clear();
    m_skinnedDrawQueue.clear();

    //renderModels();
    //renderSkinnedModels();
}

void EngineCore::clearRender() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,m_windowWidth,m_windowHeight);
    //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //glClearColor(0.027, 0.769, 0.702, 1.0f);
    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_shadowMapFBO.bind4writing();
    glClear(GL_DEPTH_BUFFER_BIT);

    m_pickingTexture.enableWriting();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(0);
}

void EngineCore::shadowPass(const meshQueue_t &queue) {
    for(const auto & matVector : queue){
        for(const auto& drawable : matVector.second) {
            renderModelShadow(drawable);
        }
    }
}

void EngineCore::shadowPass(const skinnedMeshQueue_t &queue) {
    for(const auto & matVector : queue){
        const boneTransfoms_t boneTransforms = matVector.first.second;
        m_shadowMapShader.setBoneTransforms(boneTransforms);

        for(const auto& drawable : matVector.second) {
            renderModelShadow(drawable);
        }
    }
}

void EngineCore::renderModelShadow(const Drawable& drawable) {
    glm::mat4 mashMatrix = drawable.object.transformation.getMatrix();

    glm::mat4 wvp = m_spotLights->at(0).getWVP(mashMatrix);
    m_shadowMapShader.setWVP(wvp);
    m_shadowMapShader.setWorld(mashMatrix);

    renderMesh(*drawable.mesh);
}

void EngineCore::renderModelShadow(const SkinnedDrawable& drawable) {
    glm::mat4 mashMatrix = drawable.object.transformation.getMatrix();

    glm::mat4 wvp = m_spotLights->at(0).getWVP(mashMatrix);
    m_shadowMapShader.setWVP(wvp);
    m_shadowMapShader.setWorld(mashMatrix);

    renderMesh(*drawable.mesh);
}

void EngineCore::lightingPass(const meshQueue_t &queue) {

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

void EngineCore::lightingPass(const skinnedMeshQueue_t &queue) {
    for(const auto & matVector : queue){
        const Material* material = matVector.first.first;
        const boneTransfoms_t boneTransforms = matVector.first.second;

        m_lightingShader.setBoneTransforms(boneTransforms);

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

void EngineCore::renderModelLight(const Drawable& drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_lightingShader.setWVP(wvp);
    m_lightingShader.setWorld(objectTransform);

    // Light point of view
    glm::mat4 light_wvp = m_spotLights->at(0).getWVP(objectTransform);
    m_lightingShader.setLightWVP(light_wvp);

    // Setup dir light
    m_lightingShader.setDirectionalLight(*m_dirLight);

    m_lightingShader.setWorldCameraPos(m_gameCamera->getPosition());

    m_lightingShader.setSpotLights(m_spotLightsCount, *m_spotLights);
    m_lightingShader.setPointLights(m_pointLightsCount, *m_pointLights);

    m_lightingShader.setColorMod(drawable.object.colorMod);

    renderMesh(*drawable.mesh);
}

void EngineCore::renderModelLight(const SkinnedDrawable& drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_lightingShader.setWVP(wvp);
    m_lightingShader.setWorld(objectTransform);

    // Light point of view
    glm::mat4 light_wvp = m_spotLights->at(0).getWVP(objectTransform);
    m_lightingShader.setLightWVP(light_wvp);

    // Setup dir light
    m_lightingShader.setDirectionalLight(*m_dirLight);

    m_lightingShader.setWorldCameraPos(m_gameCamera->getPosition());

    m_lightingShader.setSpotLights(m_spotLightsCount, *m_spotLights);
    m_lightingShader.setPointLights(m_pointLightsCount, *m_pointLights);

    m_lightingShader.setColorMod(drawable.object.colorMod);

    renderMesh(*drawable.mesh);
}

inline void EngineCore::renderMesh(const MeshInfo &mesh) {
    glDrawElementsBaseVertex(GL_TRIANGLES,
                             static_cast<GLsizei>(mesh.indicesCount),
                             GL_UNSIGNED_INT,
                             (void *)(mesh.indicesOffset * sizeof(uint32_t)),
                             static_cast<GLint>(mesh.verticesOffset));
}

void EngineCore::pickingPass(const meshQueue_t &queue) {
    for(const auto & matVector : queue){
        for(const auto& drawable : matVector.second) {
            renderModelPicking(drawable);
        }
    }
}

void EngineCore::pickingPass(const skinnedMeshQueue_t &queue) {
    for(const auto & matVector : queue){

        const boneTransfoms_t boneTransforms = matVector.first.second;
        m_pickingShader.setBoneTransforms(boneTransforms);

        for(const auto& drawable : matVector.second) {
            renderModelPicking(drawable);
        }
    }
}

void EngineCore::renderModelPicking(const Drawable& drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_pickingShader.setWVP(wvp);
    m_pickingShader.setObjectIndex(drawable.object.index+1);
    m_pickingShader.setObjectFlags(0);

    renderMesh(*drawable.mesh);
}

void EngineCore::renderModelPicking(const SkinnedDrawable& drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_pickingShader.setWVP(wvp);
    m_pickingShader.setObjectIndex(drawable.object.index+1);
    m_pickingShader.setObjectFlags(PickingTexture::SKINNED);

    renderMesh(*drawable.mesh);
}

void EngineCore::debugPass(const EngineCore::meshQueue_t &queue) {
    for(const auto & matVector : queue){
        for(const auto& drawable : matVector.second) {
            renderModelDebug(drawable);
        }
    }
}

void EngineCore::debugPass(const EngineCore::skinnedMeshQueue_t &queue) {
    for(const auto & matVector : queue){
        const boneTransfoms_t boneTransforms = matVector.first.second;

        m_debugShader.setBoneTransforms(boneTransforms);

        for(const auto& drawable : matVector.second) {
            renderModelDebug(drawable);
        }
    }
}

void EngineCore::renderModelDebug(const Drawable &drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_debugShader.setWVP(wvp);
    m_debugShader.setWorld(objectTransform);

    renderMesh(*drawable.mesh);
}

void EngineCore::renderModelDebug(const SkinnedDrawable &drawable) {
    glm::mat4 objectTransform = drawable.object.transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(objectTransform);
    m_debugShader.setWVP(wvp);
    m_debugShader.setWorld(objectTransform);

    renderMesh(*drawable.mesh);
}

void EngineCore::renderTerrain() {
    glm::mat4 vp = m_gameCamera->getVP();
    m_terrainShader.setWVP(vp);

    m_terrain->bindBlendingTextures();

    // Setup dir light
    m_terrainShader.setDirectionalLight(*m_dirLight);

    auto meshIter = m_terrain->meshIter();

    while(meshIter) {
        auto mesh = *meshIter;
        renderMesh(mesh);
        meshIter++;
    }
 }

void EngineCore::renderSkybox() {
    glm::mat4 vp = m_gameCamera->getVPNoTranslate();
    m_skyboxShader.setVP(vp);

    m_skyboxColor->m_cubemapTex->bind(SKYBOX_CUBE_MAP_TEXTURE_UNIT);
    renderMesh(m_skyboxColor->m_meshes.at(0));
}
*/

void EngineCore::initGLFW(){
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw engineException("Engine couldn't initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
}

void EngineCore::setWindowSize(int32_t width, int32_t height) {
    m_windowWidth = width;
    m_windowHeight = height;

    // Need to change picking texture dimensions
    //m_pickingTexture.init(m_windowWidth, m_windowHeight);
}

EngineCore::EngineCore() {
    try {
        initGLFW();
        m_window = std::make_unique<Window>();
        m_vktCore.setWindow(m_window.get());
        m_vktCore.init();

        VktCore::EngineObject* bob = m_vktCore.createObject("bob", "meshes/bob.tecm");
        if(!bob) throw engineException("Oops, no Bob");
        m_objects.insert({bob->objectID, bob});
        //bob->model->transformation.scale(0.5);

        bobAnimatrix = Animatrix(bob->model);
        for(auto& [bodyPart, points] : bobAnimatrix.debugJointLines) {
            //m_vktCore.debugLines[Utils::enumVal(bodyPart)] = &points;
        }
        for(auto& [bodyPart, points] : bobAnimatrix.debugJointBasis) {
            m_vktCore.debugLines[-Utils::enumVal(bodyPart)] = &points;
        }

        Animatrix::Action action{};
        action.type = Animatrix::ActionType::PULLING;
        action.body = Animatrix::BodyPart::HEAD;
        action.target = {0.0f, 10.0f, 0.0f};
        bobAnimatrix.actions.emplace(0,action);

        action.body = Animatrix::BodyPart::LARM;
        action.target = {3.0f, 1.0f, 0.0f};
        bobAnimatrix.actions.emplace(1,action);

        action.body = Animatrix::BodyPart::RARM;
        action.target = {-3.0f, 1.0f, 0.0f};
        bobAnimatrix.actions.emplace(2,action);

        //action.body = Animatrix::BodyPart::RLEG;
        //action.target = {0.0f, -2.0f, 0.0f};
        //bobAnimatrix.actions.emplace(3,action);


/*
        VktCore::EngineObject* shrek = m_vktCore.createObject("meshes/shrek.tecm", "shrek");
        if(!shrek) throw engineException("Oops, no Shrek");
        m_objects.insert({shrek->objectID, shrek});

        VktCore::EngineObject* bottle = m_vktCore.createObject("meshes/WaterBottle.tecm", "bottle");
        if(!bottle) throw engineException("Oops, no bottle");
        m_objects.insert({bottle->objectID, bottle});
        m_objects[1]->model.transformation.setScale(5.0);
        m_objects[1]->model.transformation.setTranslation(1.0f, 3.0f, 0.0f);
*/

        /*
        for(uint32_t i = 0; i < 5; i++){
            VktCore::EngineObject* shrek = m_vktCore.createObject("meshes/WaterBottle.tecm", "shrek");

            if(!shrek) throw engineException("Oops, no Shrek");

            m_objects.insert({shrek->objectID, shrek});
            //obj->activeAnimation = obj->resources->animations[i].get();
            shrek->model.transformation.translate(i*1.5,0,0);
            shrek->model.transformation.rotate(0.0f,180.0f,0.0f);
        }*/

        m_window->connectKeyboard(m_keyboard);
        m_window->connectCursor(m_cursor);

        initGameCamera();
        initCursor();
        initKeyboard();

        m_isInitialized = true;

        m_gameCamera->setPosition({0.0f, 0.0f, 5.0f});
    }catch(tectonicException& e){
        fprintf(stderr, "EXCEPTION: %s", e.what());
        exit(-1);
    }
}

EngineCore::~EngineCore() {

    clean();

    /*
    // Cleanup shaders
    m_lightingShader.clean();
    m_pickingShader.clean();
    m_debugShader.clean();
    m_shadowMapShader.clean();
    m_terrainShader.clean();

    // Cleanup textures
    m_pickingTexture.clean();
    m_shadowCubeMapFBO.clean();
    m_shadowMapFBO.clean();
    */
}

void EngineCore::initKeyGroups(){
    // TODO
    //  Would be way better with external config file.
    //  Maybe in 5 years? ...maybe? :`)

    m_keyboard.addKeyGroup("controls", {
            GLFW_KEY_W,
            GLFW_KEY_S,
            GLFW_KEY_A,
            GLFW_KEY_D,
            GLFW_KEY_SPACE,
            GLFW_KEY_C,
            GLFW_KEY_LEFT_SHIFT
    }, Keyboard::KeyboardGroupFlags::EMIT_ON_RELEASE);
    m_keyboard.addKeyGroup("close", {GLFW_KEY_ESCAPE});
    m_keyboard.addKeyGroup("cursorToggle", {
            GLFW_KEY_LEFT_CONTROL,
            GLFW_KEY_RIGHT_CONTROL});
    m_keyboard.addKeyGroup("polygonToggle", {GLFW_KEY_Z});
    m_keyboard.addKeyGroup("perspectiveToggle", {GLFW_KEY_X});
    m_keyboard.addKeyGroup("debugToggle", {GLFW_KEY_V});
    m_keyboard.addKeyGroup("alphaNumerics", {
            GLFW_KEY_0,
            GLFW_KEY_1,
            GLFW_KEY_2,
            GLFW_KEY_3,
            GLFW_KEY_4,
            GLFW_KEY_5
    });
    m_keyboard.addKeyGroup("generateTerrain",{GLFW_KEY_T});

    m_keyboard.connectKeyGroup("controls", m_gameCamera->slt_keyEvent);
    m_keyboard.connectKeyGroup("close", m_window->slt_setClose);
    m_keyboard.connectKeyGroup("cursorToggle", m_window->slt_toggleCursor);
}

void EngineCore::initGameCamera() {
    m_gameCamera = std::make_unique<GameCamera>();
    m_gameCamera->setPerspectiveInfo({CAMERA_PPROJ_FOV,
                                    m_window->getRatio(),
                                    CAMERA_PPROJ_NEAR,
                                    CAMERA_PPROJ_FAR});
    m_gameCamera->createProjectionMatrix();
    m_gameCamera->setSpeed(2.5f);
}

void EngineCore::initKeyboard() {
    initKeyGroups();
}

void EngineCore::initCursor() {
    //m_window->disableCursor();
    m_cursor.sig_updatePos.connect(m_gameCamera->slt_mouseMovement);
    m_window->sig_cursorEnabled.connect(m_gameCamera->slt_cursorEnabled);
    m_window->disableCursor();
    m_window->enableCursor();
}

/*
void EngineCore::initGL() {
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
    glDebugMessageCallback(EngineCore::openGLErrorCallback, nullptr);
}

void EngineCore::initShaders() {
    m_lightingShader.init();

    m_lightingShader.enable(Shader::ShaderType::BASIC_SHADER);
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

    m_terrainShader.init();
    //m_terrainShader.enable();
    //m_terrainShader.setBlendedTextureSamples(COLOR_TEXTURE_UNIT_INDEX);

    m_skyboxShader.init();
    m_skyboxShader.enable();
    m_skyboxShader.setCubemapUnit(SKYBOX_CUBE_MAP_TEXTURE_UNIT_INDEX);
}
*/

void EngineCore::glfwErrorCallback(int, const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

