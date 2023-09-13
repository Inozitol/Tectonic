#include "model/anim/SkinnedModel.h"

SkinnedModel::SkinnedModel(const Model &model) {
    m_meshes = model.m_meshes;
    m_materials = model.m_materials;
    m_vertices = model.m_vertices;
    m_indices = model.m_indices;
    m_rootNode = model.m_rootNode;
}

SkinnedModel::SkinnedModel(const std::shared_ptr<Model>& model){
    m_meshes = model->m_meshes;
    m_materials = model->m_materials;
    m_vertices = model->m_vertices;
    m_indices = model->m_indices;
    m_rootNode = model->m_rootNode;
}

const BoneInfo *SkinnedModel::getBoneInfo(const std::string &boneName) const {
    auto it = m_boneInfoMap.find(boneName);
    if(it != m_boneInfoMap.end())
        return &it->second;
    else
        return nullptr;
}

const BoneInfo* SkinnedModel::getBoneInfo(int32_t boneID) const{
    if(boneID < m_boneInfoVec.size())
        return m_boneInfoVec.at(boneID);
    else
        return nullptr;
}

const Animation *SkinnedModel::getAnimation(uint32_t animIndex) const {
    if(animIndex < m_animations.size())
        return &m_animations.at(animIndex);
    else
        return nullptr;
}

int32_t SkinnedModel::getBoneID(const std::string& boneName, const glm::mat4& offsetMatrix) {
    int32_t boneId;
    if(m_boneInfoMap.find(boneName) == m_boneInfoMap.end()){
        BoneInfo newBoneInfo{
                m_boneCounter,
                offsetMatrix
        };
        m_boneInfoMap[boneName] = newBoneInfo;
        m_boneInfoVec.push_back(&m_boneInfoMap.at(boneName));
        boneId = m_boneCounter;
        m_boneCounter++;
    }else{
        boneId = m_boneInfoMap.at(boneName).id;
    }

    return boneId;
}

void SkinnedModel::bufferMeshes() {
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

    glEnableVertexAttribArray(BONE_ID_LOCATION);
    glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(Vertex), (void*) offsetof(Vertex, m_boneIds));

    glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
    glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_weights));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(m_indices.size() * sizeof(uint32_t)), &m_indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}

