#ifndef TECTONIC_MESHTYPES_H
#define TECTONIC_MESHTYPES_H

#include <glm/mat4x4.hpp>

#include <array>
#include <string>
#include <vector>

#include "defs/ShaderDefines.h"

#define INVALID_MATERIAL 0xFFFFFFFF

using boneTransfoms_t = std::array<glm::mat4, MAX_BONES>;

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

struct MeshInfo{
    uint32_t indicesOffset = 0;   // Mesh starts at a certain offset of model indices buffer
    uint32_t verticesOffset = 0;  // Mesh starts at a certain offset of model vertices buffer
    uint32_t indicesCount = 0;
    uint32_t matIndex = INVALID_MATERIAL; // One material can be used on multiple meshes

    MeshInfo() = default;
    MeshInfo(MeshInfo&&) = default;
    MeshInfo(MeshInfo const&) = default;
    MeshInfo& operator=(MeshInfo&&) = default;
    MeshInfo& operator=(MeshInfo const&) = default;
};

struct BoneInfo{
    int id = 0;
    glm::mat4 offset = glm::mat4(1.0f);
};

struct NodeData{
    glm::mat4 transformation;
    std::string name;
    int32_t boneID = -1;
    std::vector<NodeData> children;
};

#endif //TECTONIC_MESHTYPES_H
