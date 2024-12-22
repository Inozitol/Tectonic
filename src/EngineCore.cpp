#include "engine/EngineCore.h"

#include "engine/TecCache.h"

void EngineCore::run() {
    m_vktCore.setProjMatrix(m_gameCamera->getProjectionMatrix());

    double prevTime = glfwGetTime();
    double currTime = prevTime;
    TecCache::deltaTime = currTime - prevTime;
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

        /*
        if(ctr % 200 < 50) {
            bobAnimatrix->actions[0].target = {1.0f, 5.0f, 0.0f};
            bobAnimatrix->actions[1].target = {3.0f, 6.0f, 0.0f};
            bobAnimatrix->actions[2].target = {-3.0f, 3.0f, -6.0f};
            //bobAnimatrix->actions[3].target = {0.0f, -2.0f, 0.0f};
        } else if(ctr % 200 < 100) {
            bobAnimatrix->actions[0].target = {1.0f, 4.0f, 0.0f};
            bobAnimatrix->actions[1].target = {3.0f, 0.0f, 0.0f};
            bobAnimatrix->actions[2].target = {-3.0f, 3.0f, 6.0f};
            //bobAnimatrix->actions[3].target = {0.0f, -2.0f, -5.0f};
        } else if(ctr % 200 < 150) {
            bobAnimatrix->actions[0].target = {-1.0f, 5.0f, 0.0f};
            bobAnimatrix->actions[1].target = {3.0f, 6.0f, 0.0f};
            bobAnimatrix->actions[2].target = {-3.0f, 3.0f, 6.0f};
            //bobAnimatrix->actions[3].target = {0.0f, -2.0f, 0.0f};
        } else {
            bobAnimatrix->actions[0].target = {-1.0f, 4.0f, 0.0f};
            bobAnimatrix->actions[1].target = {3.0f, 0.0f, 0.0f};
            bobAnimatrix->actions[2].target = {-3.0f, 3.0f, -6.0f};
            //bobAnimatrix->actions[3].target = {0.0f, -2.0f, 5.0f};
        }*/
        bobAnimatrix->updateActions();

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

void EngineCore::initGLFW() {
    glfwSetErrorCallback(glfwErrorCallback);

    if(!glfwInit()) {
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
}

EngineCore::EngineCore() {
    initGLFW();
    m_window = std::make_unique<Window>();
    m_vktCore.setWindow(m_window.get());
    m_vktCore.init();

    VktCore::EngineObject *bob = m_vktCore.createObject("bob", "meshes/bob.tecm");
    if(!bob) throw engineException("Oops, no Bob");
    m_objects.insert({bob->objectID, bob});

    bobAnimatrix = std::make_unique<Animatrix>(bob->model);

    m_vktCore.imguiProcedures.emplace(0, &Animatrix::runImGui);

    for(auto &[bodyPart, points]: bobAnimatrix->debugJointLines) {
        m_vktCore.debugLines[Utils::enumVal(bodyPart)+1] = &points;
    }
    for(auto &[bodyPart, points]: bobAnimatrix->debugJointBasis) {
        m_vktCore.debugLines[-Utils::enumVal(bodyPart)] = &points;
    }
    bobAnimatrix->debugScaleFactor = 1.65f;// From trial and error

    Animatrix::Action action{};
    /*
    action.type = Animatrix::ActionType::PULLING;
    action.body = Animatrix::BodyPart::HEAD;
    action.target = {0.0f, 10.0f, 0.0f};
    bobAnimatrix->actions.emplace(0, action);
*/
    action.body = Animatrix::BodyPart::LARM;
    action.target = {3.0f, 1.0f, 0.0f};
    bobAnimatrix->actions.emplace(1, action);

    action.body = Animatrix::BodyPart::RARM;
    action.target = {-3.0f, 1.0f, 0.0f};
    bobAnimatrix->actions.emplace(2, action);

    action.body = Animatrix::BodyPart::LLEG;
    action.target = {0.0f, -5.0f, 0.0f};
    bobAnimatrix->actions.emplace(3, action);

    action.body = Animatrix::BodyPart::RLEG;
    action.target = {0.0f, -5.0f, 0.0f};
    bobAnimatrix->actions.emplace(4, action);

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

    m_window->connectKeyboard(m_keyboard);
    m_window->connectCursor(m_cursor);

    initGameCamera();
    initCursor();
    initKeyboard();

    m_isInitialized = true;

    m_gameCamera->setPosition({0.0f, 0.0f, 5.0f});
}

EngineCore::~EngineCore() {
    clean();
}

void EngineCore::initKeyGroups() {
    // TODO
    //  Would be way better with external config file.
    //  Maybe in 5 years? ...maybe? :`)

    m_keyboard.addKeyGroup("controls", {GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_SPACE, GLFW_KEY_C, GLFW_KEY_LEFT_SHIFT}, Keyboard::KeyboardGroupFlags::EMIT_ON_RELEASE);
    m_keyboard.addKeyGroup("close", {GLFW_KEY_ESCAPE});
    m_keyboard.addKeyGroup("cursorToggle", {GLFW_KEY_LEFT_CONTROL,
                                            GLFW_KEY_RIGHT_CONTROL});
    m_keyboard.addKeyGroup("polygonToggle", {GLFW_KEY_Z});
    m_keyboard.addKeyGroup("perspectiveToggle", {GLFW_KEY_X});
    m_keyboard.addKeyGroup("debugToggle", {GLFW_KEY_V});
    m_keyboard.addKeyGroup("alphaNumerics", {GLFW_KEY_0,
                                             GLFW_KEY_1,
                                             GLFW_KEY_2,
                                             GLFW_KEY_3,
                                             GLFW_KEY_4,
                                             GLFW_KEY_5});
    m_keyboard.addKeyGroup("generateTerrain", {GLFW_KEY_T});

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
