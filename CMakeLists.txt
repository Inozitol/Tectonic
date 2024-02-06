cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/extern)

add_executable(Tectonic
        src/main.cpp
        src/VulkanCore.cpp
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
        src/Terrain.cpp
        src/TerrainShader.cpp
        src/Texture.cpp
        src/Transformation.cpp
        src/utils.cpp
        src/Window.cpp
        src/VulkanStucts.cpp
        src/VulkanUtils.cpp
        src/SkyboxShader.cpp
        src/VulkanDescriptors.cpp
        src/VulkanPipelines.cpp
        src/VulkanDestructQueue.cpp
        src/VulkanLoader.cpp src/VulkanTypes.cpp)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic OpenGL::GL)
target_link_libraries(Tectonic assimp)
target_link_libraries(Tectonic Vulkan::Vulkan)
target_link_libraries(Tectonic vkbootstrap)
target_link_libraries(Tectonic vma)
target_link_libraries(Tectonic stb)
target_link_libraries(Tectonic glad)
target_link_libraries(Tectonic imgui)
target_link_libraries(Tectonic fastgltf)

include_directories(include)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE GLSL_SOURCE
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.comp")

foreach(GLSL ${GLSL_SOURCE})
    message(STATUS "BUILDING GLSL SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(${PROJECT_NAME} Shaders)