#include "model/AssimpLoader.h"

std::shared_ptr<Model> AssimpLoader::loadModel(const std::string &modelFile) {
    std::shared_ptr<Model> newModel = std::make_shared<Model>();
    m_scene = m_importer.ReadFile(modelFile, ASSIMP_FLAGS);
    if(!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode){
        throw modelLoaderException("Unable to load model ", m_importer.GetErrorString());
    }
    m_modelDirectory = modelFile.substr(0, modelFile.find_last_of('/'));
    loadMaterials(newModel);
    loadNodes(newModel);
    loadMeshes(newModel);

    return newModel;
}

std::shared_ptr<SkinnedModel> AssimpLoader::loadSkinnedModel(const std::string &modelFile, const std::string& animationFile) {
    std::shared_ptr<SkinnedModel> newModel = std::make_shared<SkinnedModel>();
    m_scene = m_importer.ReadFile(modelFile, ASSIMP_FLAGS);
    if(!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode){
        throw modelLoaderException("Unable to load model ", m_importer.GetErrorString());
    }
    m_modelDirectory = modelFile.substr(0, modelFile.find_last_of('/'));
    loadMaterials(newModel);
    loadNodes(newModel);
    loadMeshes(newModel);

    loadBones(newModel);

    if(!animationFile.empty()){
        m_importer.FreeScene();

        m_scene = m_importer.ReadFile(animationFile, ASSIMP_FLAGS);
        if(!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode){
            throw modelLoaderException("Unable to load animations ", m_importer.GetErrorString());
        }
    }

    loadAnimations(newModel);

    return newModel;
}

void AssimpLoader::loadAnimations(const std::shared_ptr<SkinnedModel>& model) {
    assert(m_scene);

    model->m_animations.reserve(m_scene->mNumAnimations);
    for(uint32_t i = 0; i < m_scene->mNumAnimations; i++){
        const aiAnimation* animation = m_scene->mAnimations[i];
        model->m_animations.emplace_back(animation->mDuration, animation->mTicksPerSecond);
        loadMissingBones(animation, model);
    }
}

void AssimpLoader::loadMissingBones(const aiAnimation *animation, const std::shared_ptr<SkinnedModel>& model) {
    assert(animation);

    for(uint32_t i = 0; i < animation->mNumChannels; i++){
        const aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();
        int32_t boneID = model->getBoneID(boneName, glm::mat4(0.0f));

        NodeData* boneNode = model->findNode(boneName);
        boneNode->boneID = boneID;

        model->m_animations.back().insertBone(createBone(channel, boneID));
    }
}

Bone AssimpLoader::createBone(const aiNodeAnim *channel, int32_t boneID) {
    Bone newBone;
    newBone.m_name = channel->mNodeName.C_Str();
    newBone.m_id = boneID;

    newBone.m_positionCount = channel->mNumPositionKeys;
    for(uint32_t posIndex = 0; posIndex < newBone.m_positionCount; posIndex++){
        aiVector3D aiPos = channel->mPositionKeys[posIndex].mValue;
        double timestamp = channel->mPositionKeys[posIndex].mTime;
        KeyPosition key{
                aiVecToGLM(aiPos),
                timestamp
        };
        newBone.m_positions.insert({key.timestamp, key});
    }
    newBone.m_lastTimestamp = fmax(newBone.m_lastTimestamp, newBone.m_positions.rbegin()->first);

    newBone.m_rotationCount = channel->mNumRotationKeys;
    for(uint32_t rotIndex = 0; rotIndex < newBone.m_rotationCount; rotIndex++){
        aiQuaternion aiOrient = channel->mRotationKeys[rotIndex].mValue;
        double timestamp = channel->mRotationKeys[rotIndex].mTime;
        KeyRotation key{
                aiQuatToGLM(aiOrient),
                timestamp
        };
        newBone.m_rotations.insert({key.timestamp,key});
    }
    newBone.m_lastTimestamp = fmax(newBone.m_lastTimestamp, newBone.m_rotations.rbegin()->first);

    newBone.m_scalingCount = channel->mNumScalingKeys;
    for(uint32_t scaleIndex = 0; scaleIndex < newBone.m_scalingCount; scaleIndex++){
        aiVector3D aiScale = channel->mScalingKeys[scaleIndex].mValue;
        double timestamp = channel->mScalingKeys[scaleIndex].mTime;
        KeyScale key{
                aiVecToGLM(aiScale),
                timestamp
        };
        newBone.m_scales.insert({key.timestamp,key});
    }
    newBone.m_lastTimestamp = fmax(newBone.m_lastTimestamp, newBone.m_scales.rbegin()->first);

    return newBone;
}

void AssimpLoader::loadBones(const std::shared_ptr<SkinnedModel>& model) {
    assert(m_scene);

    for(uint32_t i = 0; i < m_scene->mNumMeshes; i++){
        const aiMesh* mesh = m_scene->mMeshes[i];
        loadBoneWeights(mesh, i, model);
    }

}

void AssimpLoader::loadBoneWeights(const aiMesh *mesh, uint32_t meshIndex, const std::shared_ptr<SkinnedModel>& model) {
    assert(mesh);
    assert(mesh->mNumBones <= MAX_BONES);
    assert(m_scene);

    for(int32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++){
        const aiBone* bone = mesh->mBones[boneIndex];

        int32_t boneID = model->getBoneID(bone->mName.C_Str(), aiMatToGLM(bone->mOffsetMatrix));
        assert(boneIndex >= 0);

        NodeData* boneNode = model->findNode(bone->mName.C_Str());
        boneNode->boneID = boneID;

        for(uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++){
            uint32_t vertexID = bone->mWeights[weightIndex].mVertexId;
            vertexID += model->m_meshes.at(meshIndex).verticesOffset;
            float weight = bone->mWeights[weightIndex].mWeight;
            assert(vertexID <= model->m_vertices.size());
            loadBoneToVertex(model->m_vertices.at(vertexID), boneID, weight);
        }
    }

    std::cout << "Loaded " << mesh->mNumBones << " bones for mesh " << mesh->mName.C_Str() << std::endl;
}

void AssimpLoader::loadBoneToVertex(Vertex &vertex, int32_t boneID, float weight) {
    //if(weight == 0.0f)
    //    return;
    for(uint8_t i = 0; i < MAX_BONES_INFLUENCE; i++){
        //assert(vertex.m_boneIds[i] != boneID);

        if(vertex.m_boneIds[i] < 0){
            vertex.m_boneIds[i] = boneID;
            vertex.m_weights[i] = weight;
            return;
        }
    }
    throw modelException("Exceeded maximum amount of bones per vertex");
}

void AssimpLoader::loadMaterials(const std::shared_ptr<Model>& model) {
    assert(m_scene);

    if(!m_scene->HasMaterials())
        return;

    model->m_materials.reserve(m_scene->mNumMaterials);

    for(uint32_t i = 0; i < m_scene->mNumMaterials; i++){
        const aiMaterial* material = m_scene->mMaterials[i];
        model->m_materials.emplace_back(createMaterial(material));
    }
}

Material AssimpLoader::createMaterial(const aiMaterial *material) {
    assert(m_scene);
    assert(!m_modelDirectory.empty());

    Material newMaterial;

    // Load material name
    aiString name;
    if(material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        newMaterial.m_name = name.C_Str();

    // Load material colors
    aiColor3D clr(0.0f, 0.0f, 0.0f);    // Holder for colors
    float flt = 0.0f;                            // Holder for floats

    if(material->Get(AI_MATKEY_COLOR_AMBIENT, clr) == AI_SUCCESS){
        newMaterial.m_ambientColor = aiColToGLM(clr);
    }
    if(material->Get(AI_MATKEY_COLOR_DIFFUSE, clr) == AI_SUCCESS){
        newMaterial.m_diffuseColor = aiColToGLM(clr);
    }
    if(material->Get(AI_MATKEY_COLOR_SPECULAR, clr) == AI_SUCCESS){
        newMaterial.m_specularColor = aiColToGLM(clr);
    }
    if(material->Get(AI_MATKEY_SHININESS, flt) == AI_SUCCESS){
        newMaterial.m_shininess = flt;
    }

    // Load textures
    newMaterial.m_diffuseTexture = createTexture(material, aiTextureType_DIFFUSE);
    newMaterial.m_specularTexture = createTexture(material, aiTextureType_SPECULAR);
    newMaterial.m_normalTexture = createTexture(material, aiTextureType_NORMALS);

    return newMaterial;
}

std::shared_ptr<Texture> AssimpLoader::createTexture(const aiMaterial *material, aiTextureType type) {
    assert(m_scene);

    if(material->GetTextureCount(type) > 0){
        aiString path;
        if(material->GetTexture(type, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);
            const aiTexture* texture;
            if((texture = m_scene->GetEmbeddedTexture(path.C_Str()))){
                std::string hint = texture->achFormatHint;
                if(hint == "png" || hint == "jpg"){
                    if(texture->mHeight == 0){
                        return std::make_shared<Texture>(GL_TEXTURE_2D, (u_char*)texture->pcData, texture->mWidth, 0, p);
                    }
                }
            }else{
                assert(!m_modelDirectory.empty());

                if (p.substr(0, 2) == ".\\") {
                    p = p.substr(2, p.size() - 2);
                }
                std::string fullpath;
                fullpath.append(m_modelDirectory).append("/").append(p);
                return std::make_shared<Texture>(GL_TEXTURE_2D, fullpath);
            }
        }
    }
    return nullptr;
}

uint32_t processNode(const aiNode* node, NodeData& nodeDest){
    uint32_t nodeCount = 0;
    nodeDest.name = node->mName.C_Str();
    nodeDest.transformation = AssimpLoader::aiMatToGLM(node->mTransformation);

    nodeCount += node->mNumChildren;

    for(uint32_t i = 0; i < node->mNumChildren; i++){
        NodeData newNode;
        nodeCount += processNode(node->mChildren[i], newNode);
        nodeDest.children.push_back(newNode);
    }
    return nodeCount;
}


void AssimpLoader::loadNodes(const std::shared_ptr<Model>& model) {
    assert(m_scene);
    assert(m_scene->mRootNode);

    if(m_scene->mRootNode->mNumChildren == 0){
        model->m_rootNode.name = m_scene->mRootNode->mName.C_Str();
        model->m_rootNode.transformation = aiMatToGLM(m_scene->mRootNode->mTransformation);
        return;
    }

    model->m_nodeCount = processNode(m_scene->mRootNode, model->m_rootNode);
    return;

    std::stack<const aiNode*> nodeStack;
    nodeStack.push(m_scene->mRootNode);

    std::stack<NodeData*> currentNodeStack;
    currentNodeStack.push(&model->m_rootNode);

    while(!nodeStack.empty()){
        const aiNode* aNode = nodeStack.top();
        nodeStack.pop();

        NodeData* currNode = currentNodeStack.top();
        currNode->name = aNode->mName.C_Str();
        currNode->transformation = aiMatToGLM(aNode->mTransformation);

        if(aNode->mNumChildren == 0){
            currentNodeStack.pop();
            continue;
        }else{
            currNode->children.emplace_back();
            currentNodeStack.push(&currNode->children.back());
        }

        for(uint32_t i = 0; i < aNode->mNumChildren; i++){
            nodeStack.push(aNode->mChildren[i]);
        }
    }
}

void AssimpLoader::loadMeshes(const std::shared_ptr<Model>& model) {
    assert(m_scene);

    int32_t indicesCount = 0;
    int32_t verticesCount = 0;

    for(uint32_t i = 0; i < m_scene->mNumMeshes; i++){
        const aiMesh* mesh = m_scene->mMeshes[i];
        loadMesh(mesh, model, indicesCount, verticesCount);
    }
}

void AssimpLoader::loadMesh(const aiMesh *mesh, const std::shared_ptr<Model>& model, int32_t& indicesCount, int32_t& verticesCount) {
    assert(m_scene);

    MeshInfo meshInfo;
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

    for(uint32_t i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];
        for(uint32_t j = 0; j < face.mNumIndices; j++){
            indices.push_back(face.mIndices[j]);
        }
    }

    if(mesh->mMaterialIndex >= 0){
        meshInfo.matIndex = mesh->mMaterialIndex;
    }

    meshInfo.indicesOffset = indicesCount;
    meshInfo.verticesOffset = verticesCount;
    meshInfo.indicesCount = indices.size();

    indicesCount += indices.size();
    verticesCount += vertices.size();

    model->m_vertices.insert(model->m_vertices.end(), vertices.begin(), vertices.end());
    model->m_indices.insert(model->m_indices.end(), indices.begin(), indices.end());

    model->m_meshes.push_back(meshInfo);
}

glm::mat4 AssimpLoader::aiMatToGLM(const aiMatrix4x4t<ai_real>& assimpMat) {
    glm::mat4 glmMat;
    glmMat[0][0] = assimpMat.a1; glmMat[1][0] = assimpMat.a2; glmMat[2][0] = assimpMat.a3; glmMat[3][0] = assimpMat.a4;
    glmMat[0][1] = assimpMat.b1; glmMat[1][1] = assimpMat.b2; glmMat[2][1] = assimpMat.b3; glmMat[3][1] = assimpMat.b4;
    glmMat[0][2] = assimpMat.c1; glmMat[1][2] = assimpMat.c2; glmMat[2][2] = assimpMat.c3; glmMat[3][2] = assimpMat.c4;
    glmMat[0][3] = assimpMat.d1; glmMat[1][3] = assimpMat.d2; glmMat[2][3] = assimpMat.d3; glmMat[3][3] = assimpMat.d4;
    return glmMat;
}

glm::vec3 AssimpLoader::aiVecToGLM(const aiVector3D& vec){
    return {vec.x, vec.y, vec.z};
}

glm::quat AssimpLoader::aiQuatToGLM(const aiQuaternion& orientation){
    return {orientation.w, orientation.x, orientation.y, orientation.z};
}

glm::vec3 AssimpLoader::aiColToGLM(const aiColor3D& col){
    return {col.r, col.g, col.b};
}

