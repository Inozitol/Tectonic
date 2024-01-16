cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Tectonic
        src/main.cpp
        src/glad.c
        src/VulkanCore.cpp
        src/VkBootstrap.cpp
        src/Animation.cpp
        src/Animator.cpp
        src/AssimpLoader.cpp
        src/Bone.cpp
        src/Camera.cpp
        src/CubemapTexture.cpp
        src/Cursor.cpp
        src/DebugShader.cpp
        src/EngineCore.cpp
        src/GameCamera.cpp
        src/Keyboard.cpp
        src/LightingShader.cpp
        src/LODManager.cpp
        src/Logger.cpp
        src/Material.cpp
        src/Model.cpp
        src/PickingShader.cpp
        src/PickingTexture.cpp
        src/Scene.cpp
        src/Shader.cpp
        src/ShadowCubeMapFBO.cpp
        src/ShadowMapFBO.cpp
        src/ShadowMapShader.cpp
        src/SkinnedModel.cpp
        src/Skybox.cpp
        src/stb_image.cpp
        src/Terrain.cpp
        src/TerrainShader.cpp
        src/Texture.cpp
        src/Transformation.cpp
        src/utils.cpp
        src/Window.cpp
        src/VulkanStucts.cpp
        src/VulkanUtils.cpp
        src/SkyboxShader.cpp)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic OpenGL::GL)
target_link_libraries(Tectonic assimp)
target_link_libraries(Tectonic Vulkan::Vulkan)

include_directories(include)