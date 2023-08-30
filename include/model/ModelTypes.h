#ifndef TECTONIC_MESHTYPES_H
#define TECTONIC_MESHTYPES_H

#include <glm/mat4x4.hpp>

#include <array>
#include <string>
#include <vector>

#include "defs/ShaderDefines.h"

#define INVALID_MATERIAL 0xFFFFFFFF

struct Vertex{
    Vertex(){
        m_boneIds.fill(-1);
        m_weights.fill(0.0f);
    }

    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 m_normal = {0.0f, 1.0f, 0.0f};
    glm::vec2 m_texCoord = {0.0f, 0.0f};
    glm::vec3 m_tangent = {0.0f, 0.0f, 0.0f};
    glm::vec3 m_bitangent = {0.0f, 0.0f, 0.0f};

    std::array<int32_t, MAX_BONES_INFLUENCE> m_boneIds{};
    std::array<float, MAX_BONES_INFLUENCE> m_weights{};
};

struct Mesh{
    int32_t indicesOffset{};   // Mesh starts at a certain offset of model indices buffer
    int32_t verticesOffset{};  // Mesh starts at a certain offset of model vertices buffer
    int32_t indicesCount{};
    int32_t matIndex = INVALID_MATERIAL; // One material can be used on multiple meshes
};

struct BoneInfo{
    int id;
    glm::mat4 offset;
};

struct NodeData{
    glm::mat4 transformation;
    std::string name;
    uint32_t childCount;
    std::vector<NodeData> children;
};

#endif //TECTONIC_MESHTYPES_H
