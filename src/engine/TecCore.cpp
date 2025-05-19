#include "TecCore.h"

#define CR_HOST
#include "extern/cr/cr.h"

#include "GlobalMemory.h"
#include "engine/imgui/ImGuiHandler.h"
#include "engine/scene/World.h"
#include "math/Physics.h"
#include "vulkan/VktLayout.h"

#include <cfenv>

TecCore::TecCore() {
    feenableexcept(FE_INVALID | FE_OVERFLOW);

    VktCorePtr->init();

    /*
    VktLayout layout;
    layout.loadXml("./pipelines/layouts/metalroughness.xml");
    layout.buildLayout();

    VktPipeline pipeline;
    pipeline.loadXml("./pipelines/test.xml");
    pipeline.buildPipeline();
    */

    WindowPtr->sig_widowDimensions.connect(VktCorePtr->slt_windowResize);
    WindowPtr->connectKeyboard(keyboard);
    WindowPtr->connectCursor(cursor);

    initCursor();
    initKeyboard();
    initWorld();

    isInitialized = true;

    auto index1 = world->skinnedObjects.objects.storeData(World::WorldObject_t{ .name = "Barbwara1", .model = Model("meshes/barbwara.tecm")});
    auto index2 = world->skinnedObjects.objects.storeData(World::WorldObject_t{ .name = "Barbwara2", .model = Model("meshes/barbwara.tecm")});
    auto index3 = world->skinnedObjects.objects.storeData(World::WorldObject_t{ .name = "Barbwara3", .model = Model("meshes/barbwara.tecm")});

    /*world->skinnedObjects.objects.data(index1).model.transformation.setTranslation(0.0f, 10.0f, 0.0f);
    world->skinnedObjects.objects.data(index2).model.transformation.setTranslation(10.0f, 10.0f, 0.0f);
    world->skinnedObjects.objects.data(index3).model.transformation.setTranslation(0.0f, 10.0f, 10.0f);*/
}

TecCore::~TecCore() { clean(); }

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

        glfwPollEvents();
        PlayerPtr->frameUpdate();
        PlayerPtr->camera.createView();
        world->updateScene();

        VktCorePtr->run();
    }
}

void TecCore::clean() {
    if(isInitialized) {
        vkDeviceWaitIdle(VktCachePtr->vkDevice);
        if(world) world->clear();
        delete(world);
        VktCorePtr->clear();
        glfwTerminate();

        isInitialized = false;
    }
}

void TecCore::initKeyboard() {
    initKeyGroups();
}

void TecCore::initKeyGroups() {
    // TODO
    //  Would be way better with external config file.
    //  Maybe in 5 years? ...maybe? :`)

    keyboard.addKeyGroup("controls", {GLFW_KEY_W,
                                      GLFW_KEY_S,
                                      GLFW_KEY_A,
                                      GLFW_KEY_D,
                                      GLFW_KEY_SPACE,
                                      GLFW_KEY_C,
                                      GLFW_KEY_LEFT_SHIFT},
                                      Keyboard::KeyboardGroupFlags::EMIT_ON_RELEASE | Keyboard::KeyboardGroupFlags::STORE_HELD_KEYS);
    keyboard.addKeyGroup("close", {GLFW_KEY_ESCAPE});
    keyboard.addKeyGroup("cursorToggle", {GLFW_KEY_LEFT_CONTROL,
                                          GLFW_KEY_RIGHT_CONTROL});
    keyboard.addKeyGroup("polygonToggle", {GLFW_KEY_Z});
    keyboard.addKeyGroup("perspectiveToggle", {GLFW_KEY_X});
    keyboard.addKeyGroup("debugToggle", {GLFW_KEY_V});
    keyboard.addKeyGroup("alphaNumerics", {GLFW_KEY_0,
                                           GLFW_KEY_1,
                                           GLFW_KEY_2,
                                           GLFW_KEY_3,
                                           GLFW_KEY_4,
                                           GLFW_KEY_5});
    keyboard.addKeyGroup("toggleFly", {GLFW_KEY_F});

    keyboard.connectKeyGroup("controls", PlayerPtr->slt_keyEvent);
    keyboard.connectKeyGroup("close", WindowPtr->slt_setClose);
    keyboard.connectKeyGroup("cursorToggle", WindowPtr->slt_toggleCursor);
    keyboard.connectKeyGroup("polygonToggle", VktCorePtr->slt_polygonModeToggle);
    keyboard.connectKeyGroup("toggleFly", PlayerPtr->slt_toggleFlying);
}

void TecCore::initCursor() {
    //m_window->disableCursor();
    cursor.sig_updatePos.connect(PlayerPtr->slt_mouseMovement);
    WindowPtr->sig_cursorEnabled.connect(PlayerPtr->slt_cursorEnabled);
    //WindowPtr->disableCursor();
    WindowPtr->enableCursor();
}

void TecCore::initWorld() {
    world = new World();

    world->terrain = new Terrain();
    Terrain *terrain = world->terrain;
    terrain->flags.set(Terrain::Flags::SET_NEAREST_SIZE);
    //terrain->flags.set(Terrain::Flags::CULL_PATCHES);
    terrain->setMaxLOD(5);
    terrain->worldScale = 1.0f;
    terrain->maxRange = 20.0f;
    terrain->generateMidpoint(577, 0.95, {});
    //terrain->generateFlat(577,577);
    VktCorePtr->debugGeometry.infLines[0] = &terrain->debugGridLines;

    world->skybox = new Skybox();
    Skybox *skybox = world->skybox;
    std::string tmp_cubemapDir = "terrain/skyboxtex/sea-panorama";
    skybox->load(tmp_cubemapDir.c_str());
    skybox->writeColorSet();
}
