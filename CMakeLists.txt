cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_CXX_STANDARD 20)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(Tectonic src/main.cpp src/glad.c src/Window.cpp src/Transformation.cpp src/Camera.cpp src/Texture.cpp src/stb_image.cpp src/Mesh.cpp src/Technique.cpp src/LightingTechnique.cpp)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic OpenGL::GL)
target_link_libraries(Tectonic assimp)

include_directories(include)