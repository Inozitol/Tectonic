
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/mat4x4.hpp>
#include <cstring>
#include <csignal>
#include <GL/glext.h>
#include <glm/gtx/string_cast.hpp>

#include "exceptions.h"
#include "texture_defines.h"

#include "Window.h"
#include "Transformation.h"
#include "Camera.h"
#include "Mesh.h"
#include "LightingTechnique.h"

Window* window;

Transformation* world_transform;
Camera* camera;
Mesh* mesh = nullptr;
LightingTechnique* technique = nullptr;
DirectionalLight* light = nullptr;

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
                    camera->keyboard_event(key);
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
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEBUG_OUTPUT);
}

void main_loop(){
    //modelTransform.rotate(-90.0f, 0.0f, 0.0f);
    while(!window->should_close()){
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        static float delta = 0.01f;

        world_transform->rotate(0.0f,delta * 100, 0.0f);
        world_transform->set_translation(0.0f, -0.5f, 1.0f);

        light->calc_local_direction(world_transform->get_matrix());

        technique->set_world(world_transform->get_matrix());
        technique->set_view(camera->view_matrix());
        technique->set_projection(camera->projection_matrix());
        technique->set_directional_light(*light);
        technique->set_material(mesh->material());

        glm::mat4x4 inv_transformation = world_transform->get_inverse_matrix();
        glm::vec4 cam_world_pos = glm::vec4(camera->position(), 1.0f);
        glm::vec4 cam_local_pos = inv_transformation * cam_world_pos;
        technique->set_local_camera_pos(cam_local_pos);

        mesh->render();

        window->swap_buffers();
        glfwPollEvents();
    }
}

int main(){
    glfwSetErrorCallback(err_callback);
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);

    try {
        window = new Window("Howdy World");
    } catch (window_exception &we){
        fprintf(stderr, "Caught window_exception: %s\n", we.what());
        exit(-1);
    }

    window->set_key_callback(key_callback);
    window->make_current_context();
    window->disable_cursor();

    init_gl();

    mesh = new Mesh();
    mesh->load_mesh("meshes/ceramic_vase.obj");

    technique = new LightingTechnique();
    technique->init();
    technique->enable();
    technique->set_diffuse_texture_unit(COLOR_TEXTURE_UNIT_INDEX);
    technique->set_specular_texture_unit(SPECULAR_EXPONENT_UNIT_INDEX);

    world_transform = new Transformation();
    camera = new Camera(60.0f, window->get_ratio(), 0.01f, 100.0f);
    window->set_mouse_callback([](GLFWwindow*, double x, double y){camera->mouse_event(x,y);});

    light = new DirectionalLight();
    light->ambient_intensity = 0.1f;
    light->diffuse_intensity = 1.0f;
    light->direction = glm::vec3(1.0f, 0.0f, 0.0f);

    try {
        main_loop();
    } catch(tectonic_exception& te){
        fprintf(stderr, "ERROR: %s", te.what());
    }
    delete(window);
    glfwTerminate();
    delete(world_transform);
    delete(camera);
    delete(light);
    return 0;
}