#include <iostream>
#include <set>
#include <queue>
#include "extern/glad/glad.h"
#include "model/Model.h"

void Model::bufferMeshes() {
    if(m_VAO != -1)
        return;

    glGenVertexArrays(1, &m_VAO);

    glBindVertexArray(m_VAO);

    glGenBuffers(ARRAY_SIZE(m_buffers), m_buffers);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(m_vertices.size() * sizeof(Vertex)), &m_vertices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_texCoord));

    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_normal));

    glEnableVertexAttribArray(TANGENT_LOCATION);
    glVertexAttribPointer(TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_tangent));

    glEnableVertexAttribArray(BITANGENT_LOCATION);
    glVertexAttribPointer(BITANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_bitangent));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(m_indices.size() * sizeof(uint32_t)), &m_indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Model::eraseBuffers() {
    if(m_VAO != -1){
        glDeleteVertexArrays(1, &m_VAO);
    }
    m_VAO = -1;
    glDeleteBuffers(ARRAY_SIZE(m_buffers), m_buffers);
}

NodeData* Model::findNode(const std::string &nodeName) {
    std::queue<NodeData*> processQueue;
    processQueue.push(&m_rootNode);
    while(!processQueue.empty()){
        NodeData* node = processQueue.front();
        processQueue.pop();

        if(node->name == nodeName){
            return node;
        }
        for(auto& child : node->children){
            processQueue.push(&child);
        }
    }
    return nullptr;
}

void Model::clear() {
    eraseBuffers();
    m_vertices.clear();
    m_materials.clear();
    m_meshes.clear();
    m_indices.clear();
}

