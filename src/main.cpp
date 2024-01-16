#include "extern/glad/glad.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "exceptions.h"
#include "engine/EngineCore.h"
#include "Scene.h"
#include "model/AssimpLoader.h"

constexpr float g_roughness = 0.95;
constexpr uint32_t g_size = 1009;

Cursor g_cursor;
Keyboard g_keyboard;

EngineCore& g_renderer = EngineCore::getInstance();
std::shared_ptr<Window> window;

std::shared_ptr<Terrain> g_terrain;

Scene g_boneScene;
skinnedModelIndex_t boneM_i;
skinnedObjectIndex_t bone_i;
objectIndex_t terrainBone_i;

void switchAnimation(uint32_t key){
    SkinnedObjectData& bobObject = g_boneScene.getSkinnedObject(bone_i);
    switch(key){
        case GLFW_KEY_1:
            bobObject.animator.playAnimation(0);
            break;
        case GLFW_KEY_2:
            bobObject.animator.playAnimation(1);
            break;
        case GLFW_KEY_3:
            bobObject.animator.playAnimation(2);
            break;
        case GLFW_KEY_4:
            bobObject.animator.playAnimation(3);
            break;
        case GLFW_KEY_5:
            bobObject.animator.playAnimation(4);
            break;
        default:
            break;
    }
}

Slot<int32_t> g_slt_animationChange{[](uint32_t key){ switchAnimation(key);}};

void switchPolygonMode(){
    static bool isFill = true;

    isFill = !isFill;

    if(isFill){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
}

Slot<> g_slt_switchPolygonMode{[](){ switchPolygonMode(); }};

void redoTerrain(){
    g_terrain->generateMidpoint(g_size, g_roughness, {
            "terrain/textures/rock.png",
            "terrain/textures/dry.png",
            "terrain/textures/grass_dark.png",
            "terrain/textures/snow.jpg"});
    g_boneScene.getGameCamera()->setPosition({0.0, g_terrain->hMapLCoord(g_terrain->getCenterCoords()), 0.0});
}

Slot<> g_slt_redoTerrain{[](){ redoTerrain(); }};

void renderLoop(){
    //SkinnedObjectData& bobObject = g_boneScene.getSkinnedObject(bone_i);
    //bobObject.animator.playAnimation(3);
    while(!window->shouldClose()){
        static float counter = 0.0f;
        static double deltaTime = 0.0f;
        static double animPrevTime = glfwGetTime();
        static double fpsPrevTime = glfwGetTime();
        static uint32_t frameCounter = 0;

        double currentTime = glfwGetTime();
        deltaTime = currentTime - animPrevTime;
        animPrevTime = currentTime;
        frameCounter++;
        if(currentTime - fpsPrevTime > 1.0) {
            std::cout << "FPS: " << frameCounter << " | " << (fpsPrevTime-1) << "ms" << std::endl;
            fpsPrevTime = currentTime;
            frameCounter = 0;
        }

        //bobObject.animator.updateAnimation(deltaTime);

        glUseProgram(0);

        glm::vec3 circleCoords = {-sinf(counter)*3,1.0f,-cosf(counter)*3};

        g_boneScene.getDirectionalLight().setDirection(glm::normalize(glm::vec3{0.0f,0.0f,0.0f} - circleCoords));

        //g_boneScene.getSpotLight(0).setPosition(circleCoords);
        //g_boneScene.getSpotLight(0).setDirection(glm::normalize(glm::vec3{0.0f,0.0f,0.0f} - circleCoords));

        counter += 0.0001;

        g_boneScene.renderScene();

        window->swapBuffers();
        glfwPollEvents();
    }
}

void initScenes(){

    std::shared_ptr<GameCamera> gameCamera(new GameCamera);
    gameCamera->switchPerspective();
    gameCamera->setPerspectiveInfo({CAMERA_PPROJ_FOV,
                                    window->getRatio(),
                                    CAMERA_PPROJ_NEAR,
                                    CAMERA_PPROJ_FAR});
    gameCamera->setOrthographicInfo({SHADOW_OPROJ_LEFT,
                                     SHADOW_OPROJ_RIGHT,
                                     SHADOW_OPROJ_BOTTOM,
                                     SHADOW_OPROJ_TOP,
                                     SHADOW_OPROJ_NEAR,
                                     SHADOW_OPROJ_FAR});
    gameCamera->createProjectionMatrix();
    gameCamera->setPosition({0.0f, 1.0, 1.0});
    window->sig_cursorEnabled.connect(gameCamera->slt_cursorEnabled);

    g_cursor.sig_updatePos.connect(g_boneScene.slt_updateMousePosition);
    g_keyboard.connectKeyGroup("controls", g_boneScene.slt_receiveKeyboardButton);

    DirectionalLight& directionalLight = g_boneScene.getDirectionalLight();
    directionalLight.ambientIntensity = 0.5f;
    directionalLight.diffuseIntensity = 1.0f;
    directionalLight.setDirection(glm::vec3(0.0f, -1.0f, -1.0f));

    /*
    spotLightIndex_t spotLightIndex = g_boneScene.createSpotLight();
    SpotLight& spotLight = g_boneScene.getSpotLight(spotLightIndex);
    spotLight.diffuseIntensity = 40.0f;
    spotLight.ambientIntensity = 0.1f;
    spotLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.attenuation.linear = 0.2f;
    spotLight.attenuation.exp = 0.1f;
    spotLight.angle = 40.0f;
    spotLight.setPosition({-2.0,1.5,0.2f});
    spotLight.setDirection({1.0, -1.0, 0.0});
    */

    /*
    pointLightIndex_t pointLightIndex = g_boneScene.createPointLight();
    PointLight& pointLight = g_boneScene.getPointLight(pointLightIndex);
    pointLight.diffuseIntensity = 10.0f;
    pointLight.setPosition({-0.5,1.0,0.0f});
    pointLight.color = {1.0f, 1.0f, 1.0f};
    pointLight.attenuation.linear = 0.2;
    pointLight.attenuation.exp = 0.1;
    */
    g_boneScene.setGameCamera(gameCamera);

    g_terrain = std::make_shared<Terrain>();
    g_terrain->flags.set(Terrain::Flags::SET_NEAREST_SIZE);
    g_terrain->setMaxLOD(3);
    g_terrain->setScale(0.1);
    g_terrain->setMaxRange(20.0);
    //g_terrain->generateMidpoint(g_size, g_roughness, {
    //    "terrain/textures/rock.png",
    //    "terrain/textures/dry.png",
    //    "terrain/textures/grass_dark.png",
    //    "terrain/textures/snow.jpg"});
    g_terrain->generateFlat(g_size, g_size, "terrain/textures/grass_dark.png");
    g_terrain->setCamera(*gameCamera);
    gameCamera->setPosition({0.0, g_terrain->hMapLCoord(g_terrain->getCenterCoords()), 0.0});
    g_boneScene.insertTerrain(g_terrain);
    //modelIndex_t terrainMeshBone_i = g_boneScene.insertModel(terrainMesh);
    //terrainBone_i = g_boneScene.createObject(terrainMeshBone_i);
    //g_boneScene.getObject(terrainBone_i).transformation.setTranslation(-25.0, 0.0, -25.0);

    /*
    std::shared_ptr<Skybox> skybox = std::make_shared<Skybox>();
    skybox->init({"terrain/skyboxtex/xpos.png",
                  "terrain/skyboxtex/xneg.png",
                  "terrain/skyboxtex/ypos.png",
                  "terrain/skyboxtex/yneg.png",
                  "terrain/skyboxtex/zpos.png",
                  "terrain/skyboxtex/zneg.png"});
    g_boneScene.insertSkybox(skybox);
    */
    AssimpLoader assimpLoader;

    std::shared_ptr<SkinnedModel> boneMesh = assimpLoader.loadSkinnedModel("meshes/Walker.fbx");
    boneMesh->bufferMeshes();
    boneM_i = g_boneScene.insertSkinnedModel(boneMesh);
    bone_i = g_boneScene.createSkinnedObject(boneM_i);
    SkinnedObjectData& bobObject = g_boneScene.getSkinnedObject(bone_i);
    bobObject.transformation.setScale(0.008);

    //bobObject.transformation.setRotation(-90.0f, 0.0f, 0.0f);
    //bobObject.transformation.setTranslation(-1.0f, 0.0f, 0.0f);

    //std::shared_ptr<Model> barrelModel(new Model);
    //barrelModel->loadModelFromFile("meshes/wine_barrel.obj");
    //modelIndex_t barrel_i = g_boneScene.insertModel(barrelModel);
    //objectIndex_t barrelObj_i = g_boneScene.createObject(barrel_i);
    //g_boneScene.getObject(barrelObj_i).transformation.setTranslation(1.0f, 0.0f, 0.0f);

    //objectIndex_t bob2 = g_boneScene.createObject(boneMesh_i);
    //ObjectData& bob2Object = g_boneScene.getObject(bob2);
    //bob2Object.transformation.setScale(0.008);
    //bob2Object.transformation.setTranslation(0.2f, 0.0f, 0.0f);

    /*
    for(int32_t x = -10; x < 10; x++){
        for(int32_t y = -10; y < 10; y++){
            skinnedObjectIndex_t obj = g_boneScene.createSkinnedObject(boneM_i);
            g_boneScene.getSkinnedObject(obj).transformation.setTranslation(x/2, 0,y/2);
            //g_boneScene.getObject(obj).transformation.setScale(0.008);
            g_boneScene.getSkinnedObject(obj).transformation.setScale(0.01);
        }
    }
    */

    //g_boneScene.setWindowDimension(window->getSize());

    g_cursor.sig_updatePos.connect(g_boneScene.slt_updateCursorPos);
    g_cursor.sig_cursorPressedPos.connect(g_renderer.slt_updateCursorPressedPos);
    g_renderer.sig_objectClicked.connect(g_boneScene.slt_objectClicked);
    g_renderer.sig_skinnedObjectClicked.connect(g_boneScene.slt_skinnedObjectClicked);
}

void initKeyGroups(){
    g_keyboard.addKeyGroup("controls", {
            GLFW_KEY_W,
            GLFW_KEY_S,
            GLFW_KEY_A,
            GLFW_KEY_D,
            GLFW_KEY_SPACE,
            GLFW_KEY_C
    });
    g_keyboard.addKeyGroup("close", {GLFW_KEY_ESCAPE});
    g_keyboard.addKeyGroup("cursorToggle", {
            GLFW_KEY_LEFT_CONTROL,
            GLFW_KEY_RIGHT_CONTROL});
    g_keyboard.addKeyGroup("polygonToggle", {GLFW_KEY_Z});
    g_keyboard.addKeyGroup("perspectiveToggle", {GLFW_KEY_X});
    g_keyboard.addKeyGroup("debugToggle", {GLFW_KEY_V});
    g_keyboard.addKeyGroup("alphaNumerics", {
        GLFW_KEY_0,
        GLFW_KEY_1,
        GLFW_KEY_2,
        GLFW_KEY_3,
        GLFW_KEY_4,
        GLFW_KEY_5
    });
    g_keyboard.addKeyGroup("generateTerrain",{GLFW_KEY_T});

    g_keyboard.connectKeyGroup("close", window->slt_setClose);
    g_keyboard.connectKeyGroup("cursorToggle", window->slt_toggleCursor);
    g_keyboard.connectKeyGroup("polygonToggle", g_slt_switchPolygonMode);
    g_keyboard.connectKeyGroup("perspectiveToggle", g_boneScene.slt_togglePerspective);
    g_keyboard.connectKeyGroup("debugToggle", g_renderer.slt_toggleDebug);
    g_keyboard.connectKeyGroup("alphaNumerics", g_slt_animationChange);
    g_keyboard.connectKeyGroup("generateTerrain", g_slt_redoTerrain);
}

int main(){
    //window = g_renderer.window;

    /*
    try {
        window->connectKeyboard(g_keyboard);
        window->connectCursor(g_cursor);

        initKeyGroups();
        initScenes();

        renderLoop();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }*/

    VulkanCore &core = VulkanCore::getInstance();
    while(true) { core.draw(); }
    return 0;
}