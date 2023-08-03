#include <glm/gtx/string_cast.hpp>
#include "Terrain.h"

void Terrain::createTerrain(uint32_t dimX, uint32_t dimZ, const char* textureFile) {
    m_dimX = dimX;
    m_dimZ = dimZ;

    uint32_t verticesCount = m_dimX * m_dimZ;
    uint32_t indicesCount = (m_dimX-1)*(m_dimZ-1)*6;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Material material;

    vertices.resize(verticesCount);

    // Generate vertices and normals
    for(uint32_t i = 0; i < verticesCount; i++){
        uint32_t x = i % m_dimX;
        uint32_t z = i / m_dimZ;

        Vertex vertex;
        vertex.m_position = {x, 0.0f, z};
        vertex.m_normal = {0.0f, 1.0f, 0.0f};
        vertex.m_texCoord = {x, z};
        vertices[i] = vertex;
    }

    // Generate indices for vertices
    for (uint32_t z = 0; z < (m_dimZ-1); z++) {
        for(uint32_t x = 0; x < (m_dimX-1); x++) {
            uint32_t i = (z * m_dimX) + x;

            // First fragment
            indices.emplace_back(i);
            indices.emplace_back(i + 1 + m_dimX);
            indices.emplace_back(i + 1);

            // Second fragment
            indices.emplace_back(i);
            indices.emplace_back(i + m_dimX);
            indices.emplace_back(i + 1 + m_dimX);
        }
    }

    material.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    material.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
    material.diffuse = new Texture(GL_TEXTURE_2D, textureFile);

    m_meshes.emplace_back(vertices, indices, material);
}
