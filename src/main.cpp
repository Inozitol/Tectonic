
#include "extern/glad/glad.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/mat4x4.hpp>
#include <cstring>
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
Mesh* vase = nullptr;
LightingTechnique* technique = nullptr;
DirectionalLight* directional_light = nullptr;
PointLight point_lights[LightingTechnique::MAX_POINT_LIGHTS];
SpotLight spot_lights[LightingTechnique::MAX_SPOT_LIGHTS];

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
}

void main_loop(){
    //world_transform->set_scale(2.0f);
    //world_transform->set_translation(0.0f, -0.5f, 1.0f);

    while(!window->should_close()){
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        static float delta = 0.01f;
        static float counter = 0.0f;
        counter += 0.01;

        //world_transform->rotate(0.0f,delta * 100, 0.0f);
        //world_transform->set_translation(0.0f, -0.5f, 1.0f);

        technique->set_world(world_transform->get_matrix());
        technique->set_view(camera->view_matrix());
        technique->set_projection(camera->projection_matrix());

        directional_light->calc_local_direction(*world_transform);
        technique->set_directional_light(*directional_light);

        spot_lights[0].direction = camera->direction();
        spot_lights[0].position = camera->position();
        spot_lights[0].calc_local_direction_position(*world_transform);

        point_lights[0].position = camera->position();
        point_lights[0].calc_local_position(*world_transform);

        technique->set_point_lights(1, point_lights);
        technique->set_spot_lights(1, spot_lights);

        technique->set_local_camera_pos(world_transform->invert(camera->position()));

        technique->set_material(vase->material());

        vase->render();

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

        try {
            window = new Window("Howdy World");
        } catch (window_exception &we){
            fprintf(stderr, "%s", we.what());
            exit(-1);
        }

        window->set_key_callback(key_callback);
        window->make_current_context();
        window->disable_cursor();

        init_gl();

        vase = new Mesh();
        vase->load_mesh("meshes/wine_barrel.obj");

        technique = new LightingTechnique();
        technique->init();
        technique->enable();
        technique->set_diffuse_texture_unit(COLOR_TEXTURE_UNIT_INDEX);
        technique->set_specular_texture_unit(SPECULAR_EXPONENT_UNIT_INDEX);

        world_transform = new Transformation();
        camera = new Camera(60.0f, window->get_ratio(), 0.01f, 100.0f);
        window->set_mouse_callback([](GLFWwindow*, double x, double y){camera->mouse_event(x,y);});

        directional_light = new DirectionalLight();
        directional_light->ambient_intensity = 0.1f;
        directional_light->diffuse_intensity = 1.0f;
        directional_light->direction = glm::vec3(1.0f, 0.0f, 0.0f);

        spot_lights[0].diffuse_intensity = 20.0f;
        spot_lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
        spot_lights[0].attenuation.linear = 10.0f;
        spot_lights[0].angle = 30.0f;

        point_lights[0].diffuse_intensity = 0.0f;
        point_lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
        point_lights[0].attenuation.linear = 10.0f;

        main_loop();
    } catch(tectonic_exception& te){
        fprintf(stderr, "%s", te.what());
    }
    delete(window);
    glfwTerminate();
    delete(world_transform);
    delete(camera);
    delete(directional_light);
    return 0;
}