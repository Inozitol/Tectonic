
#include "extern/glad/glad.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/string_cast.hpp>

#include "exceptions.h"
#include "defs/TextureDefs.h"

#include "Window.h"
#include "Transformation.h"
#include "camera/GameCamera.h"
#include "model/Model.h"
#include "shader/LightingShader.h"
#include "defs/ShaderDefines.h"
#include "model/Terrain.h"
#include "Scene.h"
#include "Cursor.h"
#include "Keyboard.h"
#include "shader/PickingShader.h"

Cursor g_cursor;
Keyboard g_keyboard;

Renderer& g_renderer = Renderer::getInstance();
std::shared_ptr<Window> window;

Scene g_boneScene;
meshIndex_t boneM_i;
objectIndex_t bone_i;
objectIndex_t terrainBone_i;

int win_width, win_height;

void err_callback(int, const char* msg){
    fprintf(stderr, "Error: %s\n", msg);
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
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

void switchAnimation(uint32_t key){
    ObjectData& bobObject = g_boneScene.getObject(bone_i);
    switch(key){
        case GLFW_KEY_0:
            bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(0));
            break;
        case GLFW_KEY_1:
            bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(1));
            break;
        case GLFW_KEY_2:
            bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(2));
            break;
        case GLFW_KEY_3:
            bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(3));
            break;
        case GLFW_KEY_4:
            bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(4));
            break;
        case GLFW_KEY_5:
            bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(5));
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

void renderLoop(){
    ObjectData& bobObject = g_boneScene.getObject(bone_i);
    bobObject.animator.playAnimation(g_boneScene.getMesh(boneM_i)->getAnimation(0));
    while(!window->shouldClose()){
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

        bobObject.animator.updateAnimation(deltaTime);
        auto transforms = bobObject.animator.getFinalBoneMatrices();

        g_renderer.m_lightingShader.enable();
        for(int32_t i = 0; i < transforms.size(); i++) {
            g_renderer.m_lightingShader.setBoneTransform(i, transforms[i]);
        }

        g_renderer.m_pickingShader.enable();
        for(int32_t i = 0; i < transforms.size(); i++) {
            g_renderer.m_pickingShader.setBoneTransform(i, transforms[i]);
        }

        g_renderer.m_shadowMapShader.enable();
        for(int32_t i = 0; i < transforms.size(); i++) {
            g_renderer.m_shadowMapShader.setBoneTransform(i, transforms[i]);
        }

        g_renderer.m_debugShader.enable();
        for(int32_t i = 0; i < transforms.size(); i++) {
            g_renderer.m_debugShader.setBoneTransform(i, transforms[i]);
        }

        glUseProgram(0);

        g_boneScene.getSpotLight(0).setPosition(g_boneScene.getGameCamera()->getPosition());
        g_boneScene.getSpotLight(0).setDirection(g_boneScene.getGameCamera()->getDirection());

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
    gameCamera->setPosition({0.0f, 0.1, 0.5});
    window->sig_cursorEnabled.connect(gameCamera->slt_cursorEnabled);

    g_cursor.sig_updatePos.connect(g_boneScene.slt_updateMousePosition);
    g_keyboard.connectKeyGroup("controls", g_boneScene.slt_receiveKeyboardButton);

    DirectionalLight& directionalLight = g_boneScene.getDirectionalLight();
    directionalLight.ambientIntensity = 0.2f;
    directionalLight.diffuseIntensity = 1.0f;
    directionalLight.setDirection(glm::vec3(0.0f, -1.0f, 0.0f));

    spotLightIndex_t spotLightIndex = g_boneScene.createSpotLight();
    SpotLight& spotLight = g_boneScene.getSpotLight(spotLightIndex);
    spotLight.diffuseIntensity = 40.0f;
    spotLight.ambientIntensity = 0.1f;
    spotLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.attenuation.linear = 0.2f;
    spotLight.attenuation.exp = 0.1f;
    spotLight.angle = 30.0f;

    /*
    pointLightIndex_t pointLightIndex = g_boneScene.createPointLight();
    PointLight& pointLight = g_boneScene.getPointLight(pointLightIndex);
    pointLight.diffuseIntensity = 10.0f;
    pointLight.color = {1.0f, 1.0f, 1.0f};
    pointLight.attenuation.linear = 0.2;
    pointLight.attenuation.exp = 0.1;
    */

    std::shared_ptr<Terrain> terrainMesh(new Terrain);
    terrainMesh->createTerrain(10,10,
                               "meshes/textures/brickwall.jpg",
                               "meshes/textures/brickwall_normal.jpg");
    meshIndex_t terrainMeshBone_i = g_boneScene.insertMesh(terrainMesh);
    terrainBone_i = g_boneScene.createObject(terrainMeshBone_i);
    g_boneScene.getObject(terrainBone_i).transformation.setTranslation(-5.0, 0.0, -5.0);

    std::shared_ptr<Model> boneMesh(new Model);
    boneMesh->loadModelFromFile("meshes/shibahu.glb");
    boneM_i = g_boneScene.insertMesh(boneMesh);
    bone_i = g_boneScene.createObject(boneM_i);
    ObjectData& bobObject = g_boneScene.getObject(bone_i);
    bobObject.transformation.setScale(0.01);
    //bobObject.transformation.setRotation(-90.0f, 0.0f, 0.0f);
    bobObject.transformation.setTranslation(-1.0f, 0.0f, 0.0f);

    //std::shared_ptr<Model> barrelModel(new Model);
    //barrelModel->loadModelFromFile("meshes/wine_barrel.obj");
    //meshIndex_t barrel_i = g_boneScene.insertMesh(barrelModel);
    //objectIndex_t barrelObj_i = g_boneScene.createObject(barrel_i);
    //g_boneScene.getObject(barrelObj_i).transformation.setTranslation(1.0f, 0.0f, 0.0f);

    //objectIndex_t bob2 = g_boneScene.createObject(boneMesh_i);
    //ObjectData& bob2Object = g_boneScene.getObject(bob2);
    //bob2Object.transformation.setScale(0.008);
    //bob2Object.transformation.setTranslation(0.2f, 0.0f, 0.0f);

    /*
    for(int32_t x = -10; x < 10; x++){
        for(int32_t y = -10; y < 10; y++){
            objectIndex_t obj = g_boneScene.createObject(barrel_i);
            g_boneScene.getObject(obj).transformation.setTranslation(x/2, 0,y/2);
            //g_boneScene.getObject(obj).transformation.setScale(0.008);
            g_boneScene.getObject(obj).transformation.setScale(0.8);
        }
    }
    */

    g_boneScene.setGameCamera(gameCamera);

    g_boneScene.setWindowDimension(window->getSize());

    g_cursor.sig_updatePos.connect(g_boneScene.slt_updateCursorPos);
    g_cursor.sig_cursorPressedPos.connect(g_renderer.slt_updateCursorPressedPos);
    g_renderer.sig_objectClicked.connect(g_boneScene.slt_objectClicked);
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

    g_keyboard.connectKeyGroup("close", window->slt_setClose);
    g_keyboard.connectKeyGroup("cursorToggle", window->slt_toggleCursor);
    g_keyboard.connectKeyGroup("polygonToggle", g_slt_switchPolygonMode);
    g_keyboard.connectKeyGroup("perspectiveToggle", g_boneScene.slt_togglePerspective);
    g_keyboard.connectKeyGroup("debugToggle", g_renderer.slt_toggleDebug);
    g_keyboard.connectKeyGroup("alphaNumerics", g_slt_animationChange);
}

int main(){
    window = g_renderer.window;

    try {
        window->connectKeyboard(g_keyboard);
        window->connectCursor(g_cursor);

        initKeyGroups();
        initScenes();

        renderLoop();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }
    return 0;
}