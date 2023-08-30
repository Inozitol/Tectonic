#include <glm/gtx/string_cast.hpp>
#include "model/Terrain.h"

void Terrain::createTerrain(uint32_t dimX, uint32_t dimZ, const char* textureFile, const char* normalFile) {
    m_dimX = dimX;
    m_dimZ = dimZ;

    uint32_t verticesCount = m_dimX * m_dimZ;
    uint32_t indicesCount = (m_dimX-1)*(m_dimZ-1)*6;

    m_vertices.resize(verticesCount);

    // Generate vertices and normals
    for(uint32_t i = 0; i < verticesCount; i++){
        uint32_t x = i % m_dimX;
        uint32_t z = i / m_dimZ;

        Vertex vertex;
        vertex.m_position = {x, 0.0f, z};
        vertex.m_normal = {0.0f, 1.0f, 0.0f};
        vertex.m_texCoord = {x, z};
        m_vertices.at(i) = vertex;
    }

    m_indices.reserve(indicesCount);

    // Generate indices for vertices
    for (uint32_t z = 0; z < (m_dimZ-1); z++) {
        for(uint32_t x = 0; x < (m_dimX-1); x++) {
            uint32_t i = (z * m_dimX) + x;

            // First fragment
            m_indices.emplace_back(i);
            m_indices.emplace_back(i + 1 + m_dimX);
            m_indices.emplace_back(i + 1);

            glm::vec3 pos1 = m_vertices.at(i).m_position;
            glm::vec3 pos2 = m_vertices.at(i + 1 + m_dimX).m_position;
            glm::vec3 pos3 = m_vertices.at(i + 1).m_position;

            glm::vec2 tex1 = m_vertices.at(i).m_texCoord;
            glm::vec2 tex2 = m_vertices.at(i + 1 + m_dimX).m_texCoord;
            glm::vec2 tex3 = m_vertices.at(i + 1).m_texCoord;

            float x1 = pos2.x - pos1.x;
            float x2 = pos3.x - pos1.x;
            float y1 = pos2.y - pos1.y;
            float y2 = pos3.y - pos1.y;
            float z1 = pos2.z - pos1.z;
            float z2 = pos3.z - pos1.z;

            float s1 = tex2.x - tex1.x;
            float s2 = tex3.x - tex1.x;
            float t1 = tex2.y - tex1.y;
            float t2 = tex3.y - tex1.y;

            float r = 1.0f / (s1 * t2 - s2 * t1);
            glm::vec3 sdir((t2*x1-t1*x2)*r, (t2*y1-t1*y2)*r, (t2*z1-t1*z2)*r);
            glm::vec3 tdir((s1*x2-s2*x1)*r, (s1*y2-s2*y1)*r, (s1*z2-s2*z1)*r);

            m_vertices.at(i).m_tangent += sdir;
            m_vertices.at(i + 1 + m_dimX).m_tangent += sdir;
            m_vertices.at(i + 1).m_tangent += sdir;

            m_vertices.at(i).m_bitangent += tdir;
            m_vertices.at(i + 1 + m_dimX).m_bitangent += tdir;
            m_vertices.at(i + 1).m_bitangent += tdir;

            // Second fragment
            m_indices.emplace_back(i);
            m_indices.emplace_back(i + m_dimX);
            m_indices.emplace_back(i + 1 + m_dimX);

            pos1 = m_vertices.at(i).m_position;
            pos2 = m_vertices.at(i + m_dimX).m_position;
            pos3 = m_vertices.at(i + 1 + m_dimX).m_position;

            tex1 = m_vertices.at(i).m_texCoord;
            tex2 = m_vertices.at(i + m_dimX).m_texCoord;
            tex3 = m_vertices.at(i + 1 + m_dimX).m_texCoord;

            x1 = pos2.x - pos1.x;
            x2 = pos3.x - pos1.x;
            y1 = pos2.y - pos1.y;
            y2 = pos3.y - pos1.y;
            z1 = pos2.z - pos1.z;
            z2 = pos3.z - pos1.z;

            s1 = tex2.x - tex1.x;
            s2 = tex3.x - tex1.x;
            t1 = tex2.y - tex1.y;
            t2 = tex3.y - tex1.y;

            r = 1.0f / (s1 * t2 - s2 * t1);
            sdir = glm::vec3((t2*x1-t1*x2)*r, (t2*y1-t1*y2)*r, (t2*z1-t1*z2)*r);
            tdir = glm::vec3((s1*x2-s2*x1)*r, (s1*y2-s2*y1)*r, (s1*z2-s2*z1)*r);

            m_vertices.at(i).m_tangent += sdir;
            m_vertices.at(i + 1).m_tangent += sdir;
            m_vertices.at(i + 1 + m_dimX).m_tangent += sdir;

            m_vertices.at(i).m_bitangent += tdir;
            m_vertices.at(i + 1).m_bitangent += tdir;
            m_vertices.at(i + 1 + m_dimX).m_bitangent += tdir;

            //m_vertices.at(i).m_position += glm::vec3(0.0f,0.1f,0.0f);

        }
    }

    m_materials.resize(1);

    m_materials.at(0).m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_materials.at(0).m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_materials.at(0).m_diffuseTexture = std::make_shared<Texture>(GL_TEXTURE_2D, textureFile);
    if(normalFile){
        m_materials.at(0).m_normalTexture = std::make_shared<Texture>(GL_TEXTURE_2D, normalFile);
    }

    m_indicesCount += m_indices.size();
    m_verticesCount += m_vertices.size();

    Mesh meshInfo;
    meshInfo.indicesOffset = 0;
    meshInfo.verticesOffset = 0;
    meshInfo.indicesCount = m_indices.size();
    meshInfo.matIndex = 0;

    m_meshes.push_back(meshInfo);
    bufferMeshes();
}
