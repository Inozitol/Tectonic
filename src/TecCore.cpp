#include "engine/TecCore.h"

#include "engine/GlobalMemory.h"
#include "engine/ImGuiHandler.h"
#include "World.h"

#include <fenv.h>

TecCore::TecCore() {
    feenableexcept(FE_INVALID | FE_OVERFLOW);

    VktCorePtr->init();
    WindowPtr->sig_widowDimensions.connect(VktCorePtr->slt_windowResize);

    VktCore::EngineObject *bob = VktCorePtr->createObject("barbwara", "meshes/barbwara.tecm");
    if(!bob) throw engineException("Oops, no Barbwara");
    m_objects.insert({bob->objectID, bob});

    bobAnimatrix = std::make_unique<Animatrix>(bob->model);
    bobAnimatrix->debugScaleFactor = 1.65f;// From trial and error

    ImGuiHandler::ImGuiMenuItem animatrixItem;
    animatrixItem.name = "Animatrix";
    animatrixItem.procedure = &Animatrix::runImGui;
    ImGuiHandler::ImGuiMenus.at("VktCore").items.emplace(animatrixItem.name, animatrixItem);

    for(auto &[bodyPart, points]: bobAnimatrix->debugJointLines) {
        VktCorePtr->addDebugPointMesh(&points);
    }
    for(auto &points: bobAnimatrix->debugJointSplits) {
        VktCorePtr->addDebugPointMesh(&points);
    }
    for(auto &[bodyPart, points]: bobAnimatrix->debugJointBasis) {
        VktCorePtr->addDebugPointMesh(&points);
    }

    // Deserialize all sequences
    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator("./seqs/")) {
        std::ifstream file(dirEntry.path(), std::ios::binary);
        file.unsetf(std::ios::skipws);
        std::size_t fileSize = std::filesystem::file_size(dirEntry.path());
        auto data = SerialTypes::BinDataVec_t(fileSize);
        file.read(reinterpret_cast<char *>(data.data()), static_cast<long>(fileSize));
        bobAnimatrix->deserializeSequence(data);
    }

    Animatrix::Action action{};

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

    WindowPtr->connectKeyboard(m_keyboard);
    WindowPtr->connectCursor(m_cursor);

    initGameCamera();
    initCursor();
    initKeyboard();
    initWorld();

    m_isInitialized = true;

    gameCamera->setPosition({0.0f, 15.0f, 20.0f});
}

TecCore::~TecCore() {
    clean();
}

void TecCore::run() {
    currTime = static_cast<float>(glfwGetTime());
    prevTime = currTime;
    deltaTime = currTime - prevTime;
    uint32_t ctr = 0;
    // TODO
    //  Seems like a bad idea to use shouldClose().
    //  The m_window isn't a hamster in a wheel.
    //  Should get close bool with a signal from Window class.
    while(!VktCorePtr->shouldClose()) {
        ctr++;
        prevTime = currTime;
        currTime = static_cast<float>(glfwGetTime());
        deltaTime = (currTime - prevTime);

        bobAnimatrix->updateActions();

        glfwPollEvents();
        gameCamera->createView();
        gameCamera->updatePosition();
        world->updateScene();

        VktCorePtr->run();
    }
}

void TecCore::clean() {
    if(m_isInitialized) {
        vkDeviceWaitIdle(VktCachePtr->vkDevice);
        if(world) world->clear();
        VktCorePtr->clear();
        glfwTerminate();
        delete gameCamera;

        m_isInitialized = false;
    }
}


void TecCore::setWindowSize(int32_t width, int32_t height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void TecCore::initKeyGroups() {
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

    m_keyboard.connectKeyGroup("controls", gameCamera->slt_keyEvent);
    m_keyboard.connectKeyGroup("close", WindowPtr->slt_setClose);
    m_keyboard.connectKeyGroup("cursorToggle", WindowPtr->slt_toggleCursor);
    m_keyboard.connectKeyGroup("polygonToggle", VktCorePtr->slt_polygonModeToggle);
}

void TecCore::initGameCamera() {
    gameCamera = new GameCamera();
    gameCamera->setPerspectiveInfo({CAMERA_PPROJ_FOV,
                                    WindowPtr->getRatio(),
                                    CAMERA_PPROJ_NEAR,
                                    CAMERA_PPROJ_FAR});
    gameCamera->createProjectionMatrix();
    gameCamera->setSpeed(5.0f);
}

void TecCore::initKeyboard() {
    initKeyGroups();
}

void TecCore::initCursor() {
    //m_window->disableCursor();
    m_cursor.sig_updatePos.connect(gameCamera->slt_mouseMovement);
    WindowPtr->sig_cursorEnabled.connect(gameCamera->slt_cursorEnabled);
    WindowPtr->disableCursor();
    WindowPtr->enableCursor();
}
void TecCore::initWorld() {
    world = new World();

    world->terrain = new Terrain();
    Terrain* terrain = world->terrain;
    terrain->flags.set(Terrain::Flags::SET_NEAREST_SIZE);
    //terrain->flags.set(Terrain::Flags::CULL_PATCHES);
    terrain->setMaxLOD(4);
    terrain->worldScale = 1.0f;
    terrain->maxRange = 20.0f;
    terrain->generateMidpoint(1025, 0.95, {});
    terrain->connectCamera(*gameCamera);

    world->skybox = new Skybox();
    Skybox* skybox = world->skybox;
    std::string tmp_cubemapDir = "terrain/skyboxtex/sea-panorama";
    skybox->load(tmp_cubemapDir.c_str());
    skybox->writeColorSet();
}

/*
void TecCore::initGL() {
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
    glDebugMessageCallback(TecCore::openGLErrorCallback, nullptr);
}

void TecCore::initShaders() {
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
