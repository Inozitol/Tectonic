cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(Tectonic src/main.cpp src/glad.c src/Window.cpp src/Transformation.cpp src/Camera.cpp src/Texture.cpp src/stb_image.cpp src/Model.cpp src/Shader.cpp src/LightingShader.cpp src/ShadowMapFBO.cpp src/GameCamera.cpp src/ShadowMapShader.cpp src/utils.cpp src/Terrain.cpp src/ShadowCubeMapFBO.cpp src/Scene.cpp src/Bone.cpp src/Animation.cpp src/Animator.cpp src/Material.cpp src/PickingTexture.cpp src/Cursor.cpp include/meta/Slot.h include/meta/Signal.h src/Keyboard.cpp src/PickingShader.cpp src/Renderer.cpp include/StackedIndex.h src/DebugShader.cpp include/model/ModelTypes.h src/SkinnedModel.cpp src/AssimpLoader.cpp src/TerrainShader.cpp src/Logger.cpp src/LODManager.cpp src/CubemapTexture.cpp src/Skybox.cpp include/shader/SkyboxShader.cpp include/model/terrain/Ocean.cpp)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic OpenGL::GL)
target_link_libraries(Tectonic assimp)

include_directories(include)