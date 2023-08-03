cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_CXX_STANDARD 20)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(Tectonic src/main.cpp src/glad.c src/Window.cpp src/Transformation.cpp src/Camera.cpp src/Texture.cpp src/stb_image.cpp src/Model.cpp src/Shader.cpp src/LightingShader.cpp src/ShadowMapFBO.cpp src/GameCamera.cpp src/ShadowMapShader.cpp src/utils.cpp src/Terrain.cpp src/ShadowCubeMapFBO.cpp src/Scene.cpp)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic OpenGL::GL)
target_link_libraries(Tectonic assimp)

include_directories(include)