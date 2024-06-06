cmake_minimum_required(VERSION 3.24)
project(Tectonic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/extern)

add_executable(Tectonic
        src/main.cpp
        src/VktCore.cpp
        src/VktStructs.cpp
        src/VktUtils.cpp
        src/VktDescriptors.cpp
        src/VktPipelines.cpp
        src/VktDeletableQueue.cpp
        src/VktTypes.cpp
        src/Logger.cpp
        src/Window.cpp
        src/utils.cpp
        src/GameCamera.cpp
        src/Camera.cpp
        src/EngineCore.cpp
        src/Keyboard.cpp
        src/Cursor.cpp
        src/Transformation.cpp
        src/Model.cpp
        include/utils/Serial.h)

target_link_libraries(Tectonic glfw)
target_link_libraries(Tectonic assimp)
target_link_libraries(Tectonic Vulkan::Vulkan)
target_link_libraries(Tectonic vkbootstrap)
target_link_libraries(Tectonic vma)
target_link_libraries(Tectonic imgui)

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

add_executable(gltf2tec
        src/gltf2tec.cpp
        src/Logger.cpp)

target_link_libraries(gltf2tec stb)
target_link_libraries(gltf2tec fastgltf)

