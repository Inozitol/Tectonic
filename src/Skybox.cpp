#include "model/terrain/Skybox.h"

Skybox::Skybox() {
    // Building unit cube
    m_vertices.resize(8);

    m_vertices.at(0).m_position = {0.5, 0.5, 0.5};
    m_vertices.at(0).m_texCoord = {0.0, 1.0};
    m_vertices.at(1).m_position = {0.5, 0.5, -0.5};
    m_vertices.at(1).m_texCoord = {1.0, 1.0};
    m_vertices.at(2).m_position = {0.5, -0.5, 0.5};
    m_vertices.at(2).m_texCoord = {0.0, 0.0};
    m_vertices.at(3).m_position = {0.5, -0.5, -0.5};
    m_vertices.at(3).m_texCoord = {1.0, 0.0};
    m_vertices.at(4).m_position = {-0.5, 0.5, 0.5};
    m_vertices.at(4).m_texCoord = {0.0, 1.0};
    m_vertices.at(5).m_position = {-0.5, 0.5, -0.5};
    m_vertices.at(5).m_texCoord = {1.0, 1.0};
    m_vertices.at(6).m_position = {-0.5, -0.5, 0.5};
    m_vertices.at(6).m_texCoord = {0.0, 0.0};
    m_vertices.at(7).m_position = {-0.5, -0.5, -0.5};
    m_vertices.at(7).m_texCoord = {1.0, 0.0};

    m_indices.resize(36);
    m_indices = {
            0,3,1,
            0,2,3,
            4,0,5,
            0,1,5,
            6,2,4,
            2,0,4,
            6,7,2,
            2,7,3,
            1,3,7,
            1,7,5,
            5,7,6,
            5,6,4
    };

    MeshInfo meshInfo;
    meshInfo.indicesCount=36;
    meshInfo.indicesOffset=0;
    meshInfo.verticesOffset=0;
    meshInfo.matIndex=0;
    m_meshes.push_back(meshInfo);
}

void Skybox::init(const std::array<std::string, 6> &filenames) {
    m_cubemapTex = std::make_unique<CubemapTexture>(filenames);

    bufferMeshes();
}