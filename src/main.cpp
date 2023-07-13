
#include "extern/glad/glad.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

#include "exceptions.h"
#include "TextureDefines.h"

#include "Window.h"
#include "Transformation.h"
#include "GameCamera.h"
#include "Mesh.h"
#include "LightingShader.h"
#include "ShadowMapFBO.h"
#include "ShadowMapShader.h"
#include "ShaderDefines.h"

Window* window;

Transformation* world_transform;
GameCamera* game_camera = nullptr;
Mesh* cat1 = nullptr;
Mesh* cat2 = nullptr;
Mesh* terrain = nullptr;
LightingShader* light_technique = nullptr;
DirectionalLight* directional_light = nullptr;
PointLight point_lights[MAX_POINT_LIGHTS];
SpotLight spot_lights[MAX_SPOT_LIGHTS];
ShadowMapFBO shadow_map;
ShadowMapShader shadow_technique;

int win_width, win_height;

void err_callback(int, const char* msg){
    fprintf(stderr, "Error: %s\n", msg);
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
                    game_camera->handleKeyEvent(key);
                    break;
                default:
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
}

void shadow_pass(){
    shadow_map.bind4writing();
    glClear(GL_DEPTH_BUFFER_BIT);

    shadow_technique.enable();

    spot_lights[1].createViewMatrix();

    glm::mat4x4 world = cat1->transformation().getMatrix();
    glm::mat4x4 wvp = spot_lights[1].getProjectionMatrix() *
                      spot_lights[1].getViewMatrix() *
                      world;
    shadow_technique.setWVP(wvp);
    cat1->render();

    world = cat2->transformation().getMatrix();
    wvp = spot_lights[1].getProjectionMatrix() *
          spot_lights[1].getViewMatrix() *
          world;
    shadow_technique.setWVP(wvp);
    cat2->render();
}

void render_mesh(Mesh& mesh){
    glm::mat4x4 world = mesh.transformation().getMatrix();

    // Camera point of view
    glm::mat4x4 wvp = game_camera->getProjectionMatrix() *
                      game_camera->getViewMatrix() *
                      world;
    light_technique->setWVP(wvp);

    // Light point of view
    glm::mat4x4 light_wvp = spot_lights[1].getProjectionMatrix() *
                            spot_lights[1].getViewMatrix() *
                            world;
    light_technique->setLightWVP(light_wvp);

    // Setup directional light
    //directional_light->calcLocalDirection(mesh.transformation());
    //light_technique->setDirectionalLight(*directional_light);

    light_technique->setLocalCameraPos(mesh.transformation().invert(game_camera->getPosition()));

    spot_lights[0].calcLocalDirectionPosition(mesh.transformation());
    spot_lights[1].calcLocalDirectionPosition(mesh.transformation());

    //light_technique->setPointLights(1, point_lights);
    light_technique->setSpotLights(2, spot_lights);
    light_technique->setMaterial(mesh.material());

    mesh.render();
}

void lighting_pass(){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,win_width,win_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    light_technique->enable();

    shadow_map.bind4reading(SHADOW_TEXTURE_UNIT);

    static float delta = 0.01f;
    static float counter = 0.0f;
    counter += delta;

    //world_transform->rotate(0.0f,delta * 100, 0.0f);
    //world_transform->setTranslation(0.0f, -0.5f, 1.0f);

    //if(counter >= 1.0f || counter <= 0.0f){
    //    delta *= -1.0f;
    //}

    spot_lights[0].setDirection(game_camera->getDirection());
    spot_lights[0].position = game_camera->getPosition();

    spot_lights[1].position = glm::vec3(-sinf(counter), 1.0f, -cosf(counter));
    spot_lights[1].setDirection(glm::normalize(glm::vec3(-0.1, 0, 0) - spot_lights[1].position));

    game_camera->setPosition(spot_lights[1].position);
    game_camera->setDirection(spot_lights[1].direction());

    game_camera->createView();

    //std::cout << "Light pos: " << glm::to_string(spot_lights[1].position) << std::endl;
    //std::cout << "Light dir: " << glm::to_string(spot_lights[1].direction()) << std::endl;

    //std::cout << "Camera pos: " << glm::to_string(game_camera->getPosition()) << std::endl;
    //std::cout << "Camera dir: " << glm::to_string(game_camera->getDirection()) << std::endl;


    render_mesh(*cat1);
    render_mesh(*cat2);
    render_mesh(*terrain);
}

void render(){
    while(!window->shouldClose()){
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        shadow_pass();
        lighting_pass();
        window->swapBuffers();
        glfwPollEvents();
    }
}

int main(){
    glfwSetErrorCallback(err_callback);
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
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

        shadow_map.init(SHADOW_WIDTH, SHADOW_HEIGHT);

        terrain = new Mesh();
        terrain->loadMesh("meshes/dry_sand_terrain.glb");
        cat1 = new Mesh();
        cat1->loadMesh("meshes/concrete_cat.obj");
        cat2 = new Mesh();
        cat2->loadMesh("meshes/concrete_cat.obj");

        cat1->transformation().setTranslation(-0.2, 0.0, 0.0);

        light_technique = new LightingShader();
        light_technique->init();
        light_technique->enable();
        light_technique->setDiffuseTextureUnit(COLOR_TEXTURE_UNIT_INDEX);
        light_technique->setSpecularTextureUnit(SPECULAR_EXPONENT_UNIT_INDEX);
        light_technique->setShadowTextureUnit(SHADOW_TEXTURE_UNIT_INDEX);

        shadow_technique.init();

        world_transform = new Transformation();
        game_camera = new GameCamera(60.0f, window->getRatio(), 0.01f, 100.0f);
        window->setMouseCallback([](GLFWwindow *, double x, double y) { game_camera->handleMouseEvent(x, y); });

        directional_light = new DirectionalLight();
        directional_light->ambientIntensity = 0.1f;
        directional_light->diffuseIntensity = 1.0f;
        directional_light->direction = glm::vec3(1.0f, 0.0f, 0.0f);

        spot_lights[0].diffuseIntensity = 1.0f;
        spot_lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
        spot_lights[0].attenuation.linear = 0.2f;
        spot_lights[0].angle = 30.0f;

        spot_lights[1].diffuseIntensity = 1.0f;
        spot_lights[1].color = glm::vec3(1.0f, 1.0f, 1.0f);
        spot_lights[1].attenuation.linear = 0.2f;
        //spot_lights[1].attenuation.exp = 0.2;
        spot_lights[1].angle = 50.0f;

        point_lights[0].diffuseIntensity = 0.0f;
        point_lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
        point_lights[0].attenuation.linear = 10.0f;

        render();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }
    delete(window);
    glfwTerminate();
    delete(world_transform);
    delete(game_camera);
    delete(directional_light);
    delete(cat1);
    delete(cat2);
    delete(terrain);
    return 0;
}