#include <iostream>
#include "Model.h"

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices, const Material& material):
    m_vertices(vertices), m_indices(indices), m_material(material){
    initMesh();
}

void Mesh::initMesh() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(m_vertices.size() * sizeof(Vertex)), &m_vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(m_indices.size() * sizeof(uint32_t)), &m_indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_normal));

    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_texCoord));

    /*
    glEnableVertexAttribArray(BONE_ID_LOCATION);
    glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(Vertex), (void*) offsetof(Vertex, m_boneIds));

    glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
    glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, m_weights));
    */

    glBindVertexArray(0);
}

void Mesh::render() const{
    if(m_material.diffuse){
        m_material.diffuse->bind(COLOR_TEXTURE_UNIT);
    }
    if(m_material.specularExp){
        m_material.specularExp->bind(SPECULAR_EXPONENT_UNIT);
    }

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Model::loadModelFromFile(const std::string &filename) {
    loadModel(filename);
}

void Model::loadModel(const std::string &filename) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, ASSIMP_FLAGS);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        throw meshException("Unable to load model ", importer.GetErrorString());
    }
    m_modelDirectory = filename.substr(0, filename.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for(uint32_t i = 0; i < node->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    for(uint32_t i = 0; i < node->mNumChildren; i++){
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Material material;
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
        uint32_t matIndex = mesh->mMaterialIndex;
        aiMaterial* mat = scene->mMaterials[matIndex];
        loadTextures(mat, material, scene);
        loadColors(mat, material);
    }

    return {vertices, indices, material};
}

void Model::render() const {
    for(const auto& mesh : m_meshes){
        mesh.render();
    }
}

Material Model::material() {
    for(const auto& mesh : m_meshes){
        auto& material = mesh.m_material;
        if(material.ambientColor != glm::vec3(0.0f, 0.0f, 0.0f)){
            return material;
        }
    }
    return Material({glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f)});
}

void Model::loadTextures(const aiMaterial *aiMat, Material& material, const aiScene *scene) {
    loadDiffuseTexture(aiMat, material, scene);
    loadSpecularTexture(aiMat, material, scene);
}

void Model::loadDiffuseTexture(const aiMaterial *aiMat, Material& material, const aiScene *scene) {
    if(aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0){
        aiString path;
        if(aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);
            const aiTexture* texture;
            if((texture = scene->GetEmbeddedTexture(path.C_Str()))){
                std::string hint = texture->achFormatHint;
                if(hint == "png" || hint == "jpg"){
                    if(texture->mHeight == 0) {
                        material.diffuse = new Texture(GL_TEXTURE_2D, (u_char *) texture->pcData,
                                                                 texture->mWidth, 3);
                        //m_materials[index].diffuse->load();
                    }
                }
            }else{
                if(p.substr(0, 2) == ".\\") {
                    p = p.substr(2, p.size() - 2);
                }
                std::string fullpath;
                fullpath.append(m_modelDirectory).append("/").append(p);
                material.diffuse = new Texture(GL_TEXTURE_2D, fullpath);
            }
        }
    }
}

void Model::loadSpecularTexture(const aiMaterial *aiMat, Material& material, const aiScene *scene) {
    if(aiMat->GetTextureCount(aiTextureType_SHININESS) > 0){
        aiString path;
        if(aiMat->GetTexture(aiTextureType_SHININESS, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);
            const aiTexture* texture;
            if((texture = scene->GetEmbeddedTexture(path.C_Str()))){
                std::string hint = texture->achFormatHint;
                if(hint == "png" || hint == "jpg"){
                    if(texture->mHeight == 0){
                        material.specularExp = new Texture(GL_TEXTURE_2D, (u_char*)texture->pcData, texture->mWidth, 1);
                    }
                }
            }else {
                if (p.substr(0, 2) == ".\\") {
                    p = p.substr(2, p.size() - 2);
                }
                std::string fullpath;
                fullpath.append(m_modelDirectory).append("/").append(p);
                material.specularExp = new Texture(GL_TEXTURE_2D, fullpath);
            }
        }
    }
}

void Model::loadColors(const aiMaterial *aiMat, Material& material) {
    aiColor3D ambient_color(0.0f, 0.0f, 0.0f);
    if(aiMat->Get(AI_MATKEY_COLOR_AMBIENT, ambient_color) == AI_SUCCESS){
        material.ambientColor.r = ambient_color.r;
        material.ambientColor.g = ambient_color.g;
        material.ambientColor.b = ambient_color.b;
    }

    aiColor3D diffuse_color(0.0f, 0.0f, 0.0f);
    if(aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == AI_SUCCESS){
        material.diffuseColor.r = diffuse_color.r;
        material.diffuseColor.g = diffuse_color.g;
        material.diffuseColor.b = diffuse_color.b;
    }

    aiColor3D specular_color(0.0f, 0.0f, 0.0f);
    if(aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == AI_SUCCESS){
        material.specularColor.r = specular_color.r;
        material.specularColor.g = specular_color.g;
        material.specularColor.b = specular_color.b;
    }
}


