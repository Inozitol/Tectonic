
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
#include "Model.h"
#include "shader/LightingShader.h"
#include "shader/shadow/ShadowMapFBO.h"
#include "shader/shadow/ShadowCubeMapFBO.h"
#include "shader/shadow/ShadowMapShader.h"
#include "defs/ShaderDefines.h"
#include "Terrain.h"
#include "Scene.h"
#include "Animator.h"

Window* window;

Scene g_catScene;
Scene g_boneScene;
std::shared_ptr<LightingShader> g_lightShader;
Scene::objectIndex_t cat1_i;
Scene::objectIndex_t cat2_i;
Scene::objectIndex_t terrainCat_i;
Scene::objectIndex_t bone_i;
Scene::objectIndex_t terrainBone_i;


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

void switchPolygonMode(){
    static bool isFill = true;

    isFill = !isFill;

    if(isFill){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods){
    switch(action){
        case GLFW_PRESS:
        case GLFW_REPEAT:
            switch(key){
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(win, GLFW_TRUE);
                case GLFW_KEY_W:
                case GLFW_KEY_S:
                case GLFW_KEY_A:
                case GLFW_KEY_D:
                case GLFW_KEY_SPACE:
                case GLFW_KEY_C:
                    g_boneScene.handleKeyEvent(key);
                    break;
                default:
                    break;
                case GLFW_KEY_X:
                    g_boneScene.getGameCamera()->toggleProjection();
                    g_boneScene.getGameCamera()->createProjectionMatrix();
                    break;
                case GLFW_KEY_Z:
                    switchPolygonMode();
                    break;
            }
            break;
        default:
            break;
    }
}

void init_gl(){
    gladLoadGL();
    glfwSwapInterval(1);

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
    glDebugMessageCallback(MessageCallback, nullptr);
}

void render_loop(){
    ObjectData& bobObject = g_boneScene.getObject(bone_i);
    Animator bobAnimator(&bobObject.animation);
    while(!window->shouldClose()){
        static float deltaTime = 0.0f;
        static float lastFrame = 0.0f;

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        bobAnimator.updateAnimation(deltaTime);

        auto transforms = bobAnimator.getFinalBoneMatrices();
        for(int32_t i = 0; i < transforms.size(); i++) {
            g_lightShader->setBoneTransform(i, transforms[i]);
        }

        //static glm::vec3 circleCoefs = {0.0f, 0.5f, 0.0f};
        //circleCoefs.x = -sinf(counter);
        //circleCoefs.z = -cosf(counter);

        //directional_light->setDirection(glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - circleCoefs));
        //g_boneScene.getDirectionalLight()->setDirection(glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - circleCoefs));

        //g_catScene.getSpotLight(0)->setWorldPosition(circleCoefs);
        //g_catScene.getSpotLight(0)->setDirection(glm::normalize(glm::vec3(0.0, 0.0, 0.0) - circleCoefs));

        //cat1->transformation().setTranslation(counter, 0.0f, 0.0f);
        //g_catScene.getObject(cat1_i).setTranslation(0.0f, 0.0f, counter);

        //point_lights[0].setWorldPosition(circleCoefs);
        //point_lights[0].setWorldPosition(g_gameCamera->getPosition());

        //g_gameCamera->setWorldPosition(spot_lights[1].getWorldPosition());
        //g_gameCamera->setDirection(spot_lights[1].getDirection());

        g_boneScene.renderScene();

        window->swapBuffers();
        glfwPollEvents();
    }
}

void initScenes(){
    std::shared_ptr<LightingShader> lightingShader(new LightingShader);
    std::shared_ptr<ShadowMapShader> shadowShader(new ShadowMapShader);
    std::shared_ptr<ShadowMapFBO> shadowMapFBO(new ShadowMapFBO);
    std::shared_ptr<ShadowCubeMapFBO> shadowCubeMapFBO(new ShadowCubeMapFBO);

    lightingShader->init();
    lightingShader->enable();
    lightingShader->setDiffuseTextureUnit(COLOR_TEXTURE_UNIT_INDEX);
    lightingShader->setSpecularTextureUnit(SPECULAR_EXPONENT_UNIT_INDEX);
    lightingShader->setShadowMapTextureUnit(SHADOW_TEXTURE_UNIT_INDEX);
    lightingShader->setShadowCubeMapTextureUnit(SHADOW_CUBE_MAP_TEXTURE_UNIT_INDEX);

    shadowShader->init();

    shadowMapFBO->init(SHADOW_WIDTH, SHADOW_HEIGHT);

    shadowCubeMapFBO->init(1000);

    g_catScene.setLightingShader(lightingShader);
    g_catScene.setShadowShader(shadowShader);
    g_catScene.setShadowMapFBO(shadowMapFBO);
    g_catScene.setShadowCubeMapFBO(shadowCubeMapFBO);

    g_lightShader = lightingShader;

    std::shared_ptr<Model> catMesh(new Model);
    std::shared_ptr<Terrain> terrainMesh(new Terrain);
    catMesh->loadModelFromFile("meshes/concrete_cat.obj");
    terrainMesh->createTerrain(50,50,"meshes/textures/bricks.jpg");
    Scene::meshIndex_t catMesh_i = g_catScene.insertMesh(catMesh);
    Scene::meshIndex_t terrainMesh_i = g_catScene.insertMesh(terrainMesh);

    cat1_i = g_catScene.createObject(catMesh_i);
    cat2_i = g_catScene.createObject(catMesh_i);
    terrainCat_i = g_catScene.createObject(terrainMesh_i);

    g_catScene.getObject(cat1_i).transformation.setTranslation(-0.2, 0.0, 0.0);
    g_catScene.getObject(terrainCat_i).transformation.setTranslation(-25.0, 0.0, -25.0);

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
    window->setMouseCallback([](GLFWwindow *, double x, double y) { g_boneScene.handleMouseEvent(x, y); });
    g_catScene.setGameCamera(gameCamera);

    std::shared_ptr<DirectionalLight> directionalLight(new DirectionalLight);
    directionalLight->ambientIntensity = 0.1f;
    directionalLight->diffuseIntensity = 1.0f;
    directionalLight->setDirection(glm::vec3(1.0f, 0.0f, 0.0f));

    std::shared_ptr<SpotLight> spotLights[MAX_SPOT_LIGHTS];
    spotLights[0] = std::make_shared<SpotLight>();
    spotLights[0]->diffuseIntensity = 1.0f;
    spotLights[0]->ambientIntensity = 0.0f;
    spotLights[0]->color = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLights[0]->attenuation.linear = 0.2f;
    spotLights[0]->angle = 30.0f;

    spotLights[1] = std::make_shared<SpotLight>();
    spotLights[1]->diffuseIntensity = 2.0f;
    spotLights[1]->color = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLights[1]->attenuation.linear = 0.01f;
    spotLights[1]->angle = 30.0f;
    spotLights[1]->setWorldPosition({0.0f, 0.1, 0.5});
    spotLights[1]->setDirection({0.0f, 0.0f, -1.0f});

    std::shared_ptr<PointLight> pointLight(new PointLight);
    pointLight->diffuseIntensity = 1.0f;
    pointLight->color = {1.0f, 1.0f, 1.0f};
    pointLight->attenuation.linear = 0.2;

    g_catScene.insertDirectionalLight(directionalLight);
    g_catScene.insertSpotLight(spotLights[0]);
    //g_catScene.insertSpotLight(spotLights[1]);
    //g_catScene.insertPointLight(pointLight);

    g_catScene.setWindowDimension(window->getSize());

    // Bone Scene
    g_boneScene.setLightingShader(lightingShader);
    g_boneScene.setShadowShader(shadowShader);
    g_boneScene.setShadowMapFBO(shadowMapFBO);
    g_boneScene.setShadowCubeMapFBO(shadowCubeMapFBO);

    Scene::meshIndex_t terrainMeshBone_i = g_boneScene.insertMesh(terrainMesh);
    terrainBone_i = g_boneScene.createObject(terrainMeshBone_i);
    g_boneScene.getObject(terrainBone_i).transformation.setTranslation(-25.0, 0.0, -25.0);
    std::shared_ptr<Model> boneMesh(new Model);
    boneMesh->loadModelFromFile("meshes/boblampclean.md5mesh");
    Scene::meshIndex_t boneMesh_i = g_boneScene.insertMesh(boneMesh);
    bone_i = g_boneScene.createObject(boneMesh_i);
    ObjectData& bobObject = g_boneScene.getObject(bone_i);
    bobObject.transformation.setScale(0.005);
    //bobObject.transformation.setRotation(-90.0f, 0.0f, 0.0f);
    bobObject.transformation.setTranslation(0.0f, 0.0f, 0.0f);
    bobObject.animation = Animation("meshes/boblampclean.md5anim", boneMesh.get());
    g_boneScene.insertDirectionalLight(directionalLight);
    g_boneScene.setGameCamera(gameCamera);

    g_boneScene.setWindowDimension(window->getSize());
}

int main(){
    glfwSetErrorCallback(err_callback);
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    try {
        try {
            window = new Window("Howdy World");
        } catch (windowException &we){
            fprintf(stderr, "%s", we.what());
            exit(-1);
        }

        window->setKeyCallback(key_callback);
        window->makeCurrentContext();
        window->disableCursor();
        std::tie(win_width, win_height) = window->getSize();

        init_gl();
        initScenes();

        render_loop();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }
    delete(window);
    glfwTerminate();
    return 0;
}