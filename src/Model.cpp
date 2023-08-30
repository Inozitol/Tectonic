#include <iostream>
#include <set>
#include "model/Model.h"

void Model::loadModelFromFile(const std::string &filename) {
    loadModel(filename);
}

void Model::loadModel(const std::string &filename) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, ASSIMP_FLAGS);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        throw modelException("Unable to load model ", importer.GetErrorString());
    }
    m_modelDirectory = filename.substr(0, filename.find_last_of('/'));
    loadMaterials(scene);
    processNode(scene->mRootNode, scene, m_rootNode);
    bufferMeshes();

    if(scene->HasAnimations())
        loadAnimations(scene);
}

void Model::loadAnimations(const aiScene *scene) {
    m_animations.reserve(scene->mNumAnimations);
    for(uint32_t i = 0; i < scene->mNumAnimations; i++){
        const aiAnimation* anim = scene->mAnimations[i];
        m_animations.emplace_back(anim);
        readMissingBoneData(anim);
    }
}

void Model::readMissingBoneData(const aiAnimation* animation){
    assert(animation);
    for(uint32_t i = 0; i < animation->mNumChannels; i++){
        const aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();
        if(m_boneInfoMap.find(boneName) == m_boneInfoMap.end()){
            m_boneInfoMap[boneName].id = m_boneCounter;
            m_boneCounter++;
        }
        Bone bone(channel->mNodeName.C_Str(), m_boneInfoMap.at(channel->mNodeName.C_Str()).id, channel);
        m_animations.back().insertBone(bone);
    }
}

void Model::loadMaterials(const aiScene *scene) {
    if(!scene->HasMaterials())
        return;

    m_materials.reserve(scene->mNumMaterials);

    for(uint32_t i = 0; i < scene->mNumMaterials; i++){
        auto material = scene->mMaterials[i];
        m_materials.emplace_back(material, scene, m_modelDirectory);
    }
}

void Model::processNode(const aiNode *node, const aiScene *scene, NodeData &nodeDest) {
    assert(node);

    // Load node metadata and transformation matrix
    nodeDest.name = node->mName.C_Str();
    nodeDest.transformation = Utils::aiMatToGLM(node->mTransformation);
    nodeDest.childCount = node->mNumChildren;

    // Load node meshes
    for(uint32_t i = 0; i < node->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        addMesh(mesh, scene);
    }

    // Load children of node
    for(uint32_t i = 0; i < node->mNumChildren; i++){
        NodeData newNode;
        processNode(node->mChildren[i], scene, newNode);
        nodeDest.children.push_back(newNode);
    }
}

void Model::addMesh(aiMesh *mesh, const aiScene *scene) {
    Mesh meshInfo;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces*3);

    for(uint32_t i = 0; i < mesh->mNumVertices; i++){
        Vertex vertex{};
        vertex.m_position.x = mesh->mVertices[i].x;
        vertex.m_position.y = mesh->mVertices[i].y;
        vertex.m_position.z = mesh->mVertices[i].z;
        if(mesh->HasNormals()){
            vertex.m_normal.x = mesh->mNormals[i].x;
            vertex.m_normal.y = mesh->mNormals[i].y;
            vertex.m_normal.z = mesh->mNormals[i].z;
        }
        if(mesh->mTextureCoords[0]) {
            vertex.m_texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.m_texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        if(mesh->HasTangentsAndBitangents()){
            vertex.m_tangent.x = mesh->mTangents[i].x;
            vertex.m_tangent.y = mesh->mTangents[i].y;
            vertex.m_tangent.z = mesh->mTangents[i].z;
            vertex.m_bitangent.x = mesh->mBitangents[i].x;
            vertex.m_bitangent.y = mesh->mBitangents[i].y;
            vertex.m_bitangent.z = mesh->mBitangents[i].z;
        }

        vertices.push_back(vertex);
    }

    extractBoneWeightForVertices(vertices, mesh, scene);

    for(uint32_t i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];
        for(uint32_t j = 0; j < face.mNumIndices; j++){
            indices.push_back(face.mIndices[j]);
        }
    }

    if(mesh->mMaterialIndex >= 0){
        meshInfo.matIndex = mesh->mMaterialIndex;
    }

    meshInfo.indicesOffset = m_indicesCount;
    meshInfo.verticesOffset = m_verticesCount;
    meshInfo.indicesCount = indices.size();

    m_indicesCount += indices.size();
    m_verticesCount += vertices.size();

    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
    m_indices.insert(m_indices.end(), indices.begin(), indices.end());

    m_meshes.push_back(meshInfo);
}

void Model::setVertexBoneData(Vertex &vertex, int32_t boneId, float weight) {
    //assert(weight != 0.0f);
    if(weight == 0.0f)
        return;
    for(uint32_t i = 0; i < MAX_BONES_INFLUENCE; i++){
        assert(vertex.m_boneIds[i] != boneId);

        if(vertex.m_boneIds[i] < 0){
            vertex.m_boneIds[i] = boneId;
            vertex.m_weights[i] = weight;
            return;
        }
    }
    throw modelException("Exceeded maximum amount of bones per vertex");
}

void Model::extractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene) {
    assert(mesh->mNumBones <= MAX_BONES);

    for(int32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++){
        const aiBone* bone = mesh->mBones[boneIndex];

        int boneId = getBoneId(bone);
        assert(boneId >= 0);

        auto weights = bone->mWeights;
        uint32_t numWeights = bone->mNumWeights;

        for(uint32_t weightIndex = 0; weightIndex < numWeights; weightIndex++){
            uint32_t vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            setVertexBoneData(vertices[vertexId], boneId, weight);
        }
    }
}

int32_t Model::getBoneId(const aiBone* bone) {
    int32_t boneId;
    std::string boneName = bone->mName.C_Str();
    if(m_boneInfoMap.find(boneName) == m_boneInfoMap.end()){
        BoneInfo newBoneInfo{
            m_boneCounter,
            Utils::aiMatToGLM(bone->mOffsetMatrix)
        };
        m_boneInfoMap[boneName] = newBoneInfo;
        boneId = m_boneCounter;
        m_boneCounter++;
    }else{
        boneId = m_boneInfoMap.at(boneName).id;
    }

    return boneId;
}


void Model::bufferMeshes() {

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

void Model::calcBoneTransform(const NodeData *node, glm::mat4 parentTransform) {
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone* bone = m_animations.at(m_currentAnim).findBone(nodeName);

    if(bone){
        bone->update(m_currentAnimTime);
        nodeTransform = bone->getLocalTransform();
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    if(m_boneInfoMap.find(nodeName) != m_boneInfoMap.end()){
        int32_t boneId = m_boneInfoMap.at(nodeName).id;
        glm::mat4 offset = m_boneInfoMap.at(nodeName).offset;
        m_finalBoneMatrices.at(boneId) = globalTransform * offset;
    }

    for(int i = 0; i < node->childCount; i++){
        calcBoneTransform(&node->children.at(i), globalTransform);
    }
}


