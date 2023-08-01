#include <glm/gtx/string_cast.hpp>
#include "Terrain.h"

void Terrain::createTerrain(uint32_t dimX, uint32_t dimZ, const char* textureFile) {
    m_dimX = dimX;
    m_dimZ = dimZ;

    uint32_t verticesCount = m_dimX * m_dimZ;

    m_positions.resize(verticesCount);
    m_texCoords.resize(verticesCount);
    m_normals.resize(verticesCount);

    // Generate vertices and normals
    for(uint32_t i = 0; i < verticesCount; i++){
        uint32_t x = i % m_dimX;
        uint32_t z = i / m_dimZ;

        m_positions[i] = glm::vec3(x, 0.0f, z);
        m_texCoords[i] = glm::vec2(x, z);
        m_normals[i] = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Generate indices for vertices
    for (uint32_t z = 0; z < (m_dimZ-1); z++) {
        for(uint32_t x = 0; x < (m_dimX-1); x++) {
            uint32_t i = (z * m_dimX) + x;

            // First fragment
            m_indices.emplace_back(i);
            m_indices.emplace_back(i + 1 + m_dimX);
            m_indices.emplace_back(i + 1);

            // Second fragment
            m_indices.emplace_back(i);
            m_indices.emplace_back(i + m_dimX);
            m_indices.emplace_back(i + 1 + m_dimX);
        }
    }

    Material material;
    material.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    material.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
    material.diffuse = new Texture(GL_TEXTURE_2D, textureFile);
    m_materials.emplace_back(material);

    // Create VAO
    glGenVertexArrays(1, &m_vao);
    Utils::EphemeralVAOBind bind(m_vao);

    glGenBuffers(ARRAY_SIZE(m_buffers), m_buffers);

    basicMeshEntry entry;
    entry.materialIndex = 0;
    entry.indicesCount = (m_dimX-1)*(m_dimZ-1)*6;

    m_meshes.emplace_back(entry);

    bufferData();
}
