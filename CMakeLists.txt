cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -rdynamic")
#add_compile_options(-Wall -Wextra -pedantic)
add_definitions(-DGLM_FORCE_INTRINSICS)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DVKT_DEBUG_ALLOCATION_NAMES)

find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/extern)

add_executable(Tectonic
        src/main.cpp
        src/engine/vulkan/VktCore.cpp
        src/engine/vulkan/VktUtils.cpp
        src/engine/vulkan/VktDescriptorUtils.cpp
        src/engine/vulkan/VktPipelines.cpp
        src/engine/vulkan/VktDeletableQueue.cpp
        src/engine/vulkan/VktTypes.cpp
        src/engine/io/Window.cpp
        src/utils/Utils.cpp
        src/engine/camera/GameCamera.cpp
        src/engine/camera/Camera.cpp
        src/engine/TecCore.cpp
        src/engine/TecCore.h
        src/engine/io/Keyboard.cpp
        src/engine/io/Cursor.cpp
        src/math/Transformation.cpp
        src/engine/scene/model/Model.cpp
        src/utils/Serial.h
        src/engine/vulkan/VktBuffers.cpp
        src/engine/vulkan/VktInstantCommands.cpp
        src/engine/vulkan/VktImages.cpp
        src/engine/scene/Skybox.cpp
        src/engine/vulkan/VktCache.cpp
        src/engine/scene/model/Animatrix.cpp
        src/engine/scene/model/Animatrix.h
        src/engine/imgui/ImGuiHandler.h
        src/engine/imgui/ImGuiHandler.cpp
        src/engine/imgui/ImGuiUtils.h
        src/engine/GlobalMemory.h
        src/engine/scene/terrain/Terrain.cpp
        src/engine/scene/terrain/Terrain.h
        src/engine/scene/terrain/LODManager.cpp
        src/engine/scene/World.cpp
        src/engine/scene/World.h
        src/engine/scene/terrain/PatchManager.cpp
        src/engine/scene/terrain/PatchManager.h
        src/engine/Player.cpp
        src/engine/Player.h
        src/geometry/BB.h
        src/math/Physics.h
        src/utils/FloatLimit.h
        src/utils/Meta.h
        src/geometry/SAABB.h
        src/geometry/SAABB.cpp
        src/geometry/OBB.cpp
        src/geometry/OBB.h
        src/geometry/AABB.cpp
        src/geometry/AABB.h
        src/math/MathUtils.h
        src/utils/IndexableStorage.h
        src/engine/scene/RenderableBuffer.h
        src/engine/vulkan/VktConstraints.h
        src/engine/vulkan/VktStructs.h
        src/engine/vulkan/VktLayout.h
        src/engine/vulkan/VktLayout.cpp
)

#find_library(KTX_LIB ktx)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic Vulkan::Vulkan)
target_link_libraries(Tectonic vkbootstrap)
target_link_libraries(Tectonic GPUOpen::VulkanMemoryAllocator)
target_link_libraries(Tectonic imgui)
target_link_libraries(Tectonic ktx)
target_link_libraries(Tectonic pugixml)
target_link_libraries(Tectonic vulkan-mini-libs)

include_directories(src)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE GLSL_SOURCE
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.comp"
        "${PROJECT_SOURCE_DIR}/shaders/*.geom")

foreach(GLSL ${GLSL_SOURCE})
    message(STATUS "BUILDING GLSL SHADER")

    set(SPIRV "${GLSL}.spv")
    message(STATUS ${GLSL})

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(${PROJECT_NAME} Shaders)

add_executable(gltf2tec
        tools/gltf2tec/gltf2tec.cpp
)

target_link_libraries(gltf2tec stb)
target_link_libraries(gltf2tec fastgltf)
target_link_libraries(gltf2tec GPUOpen::VulkanMemoryAllocator)
