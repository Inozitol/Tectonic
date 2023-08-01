#include <iostream>
#include "Mesh.h"

#include "utils.h"

#define POSITION_LOCATION   0
#define TEX_COORD_LOCATION  1
#define NORMAL_LOCATION     2

Mesh::~Mesh() {
    clear();
}

void Mesh::loadMesh(const std::string &filename) {
    // Clear previous cat1
    clear();

    // Create VAO
    glGenVertexArrays(1, &m_vao);
    Utils::EphemeralVAOBind bind(m_vao);

    glBindVertexArray(m_vao);
    // Create buffers for vertices
    glGenBuffers(ARRAY_SIZE(m_buffers), m_buffers);

    const aiScene* scene = m_importer.ReadFile(filename.c_str(), ASSIMP_FLAGS);
    if(scene){
        initFromScene(scene, filename);
    }else{
        throw meshException("Unable to parse file ", filename);
    }
}

void Mesh::render() {
    // Binding VAO for render
    glBindVertexArray(m_vao);
    for(const auto& mesh : m_meshes){
        uint32_t material_index = mesh.materialIndex;

        assert(material_index < m_materials.size());

        if(m_materials[material_index].diffuse){
            m_materials[material_index].diffuse->bind(COLOR_TEXTURE_UNIT);
        }
        if(m_materials[material_index].specularExp){
            m_materials[material_index].specularExp->bind(SPECULAR_EXPONENT_UNIT);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES,
                                 static_cast<GLsizei>(mesh.indicesCount),
                                 GL_UNSIGNED_INT,
                                 (void*)(sizeof(mesh.baseIndex) * mesh.baseIndex),
                                 static_cast<GLsizei>(mesh.baseVertex));
    }
}

Material Mesh::material() {
    for(const auto& material : m_materials){
        if(material.ambientColor != glm::vec3(0.0f, 0.0f, 0.0f)){
            return material;
        }
    }
    return Material({glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f)});
}

void Mesh::initFromScene(const aiScene *scene, const std::string &filename) {
    m_meshes.resize(scene->mNumMeshes);
    m_materials.resize(scene->mNumMaterials);

    initMeshesIndexes(scene);
    initAllMeshes(scene);
    initMaterials(scene, filename);

    bufferData();
}

void Mesh::clear(){
    {
        Utils::EphemeralVAOBind bind(m_vao);

        // Delete OpenGL buffers
        glDeleteBuffers(1, &m_buffers[POS_VB]);
        glDeleteBuffers(1, &m_buffers[TEXCOORD_VB]);
        glDeleteBuffers(1, &m_buffers[NORMAL_VB]);
        glDeleteBuffers(1, &m_buffers[INDEX_BUFFER]);
    }

    // Delete VAO
    glDeleteVertexArrays(1, &m_vao);
}

void Mesh::initMeshesIndexes(const aiScene *scene) {
    uint32_t c_vertices = 0, c_indices = 0;

    // Init indexes to vertices and indices
    for(uint32_t i = 0; i < m_meshes.size(); i++){
        m_meshes[i].materialIndex = scene->mMeshes[i]->mMaterialIndex;
        m_meshes[i].indicesCount  = scene->mMeshes[i]->mNumFaces * 3;
        m_meshes[i].baseVertex    = c_vertices;
        m_meshes[i].baseIndex     = c_indices;

        c_vertices += scene->mMeshes[i]->mNumVertices;
        c_indices  += m_meshes[i].indicesCount;
    }
}

void Mesh::initAllMeshes(const aiScene *scene) {
    for(uint32_t i = 0; i < m_meshes.size(); i++){
        const aiMesh* mesh = scene->mMeshes[i];
        initSingleMesh(mesh);
    }
}

void Mesh::initSingleMesh(const aiMesh *mesh) {
    const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

    // Init vertices
    for(uint32_t i = 0; i < mesh->mNumVertices; i++){
        const aiVector3D& pos       = mesh->mVertices[i];
        const aiVector3D& normal    = mesh->mNormals[i];
        const aiVector3D& tex_coord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero3D;

        m_positions.emplace_back(pos.x, pos.y, pos.z);
        m_normals.emplace_back(normal.x, normal.y, normal.z);
        m_texCoords.emplace_back(tex_coord.x, tex_coord.y);
    }

    // Init indices
    for(uint32_t i = 0; i < mesh->mNumFaces; i++){
        const aiFace& face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        m_indices.emplace_back(face.mIndices[0]);
        m_indices.emplace_back(face.mIndices[1]);
        m_indices.emplace_back(face.mIndices[2]);
    }
}

void Mesh::initMaterials(const aiScene *scene, const std::string &filename) {
    std::string::size_type slash_index = filename.find_last_of('/');
    std::string dir;

    if(slash_index == std::string::npos){
        dir = ".";
    }else if(slash_index == 0){
        dir = "/";
    }else{
        dir = filename.substr(0, slash_index);
    }

    for(uint32_t i = 0; i < scene->mNumMaterials; i++){
        const aiMaterial* material = scene->mMaterials[i];
        loadTextures(dir, material, i, scene);
        loadColors(material, i);
    }
}

void Mesh::bufferData() {
    // Buffers for vertices
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(m_positions[0]) * m_positions.size()), &m_positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Buffers for texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(m_texCoords[0]) * m_texCoords.size()), &m_texCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Buffers for normals
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(m_normals[0]) * m_normals.size()), &m_normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Buffers for indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(m_indices[0]) * m_indices.size()), &m_indices[0], GL_STATIC_DRAW);
}

Transformation& Mesh::transformation() {
    return m_transformation;
}

void Mesh::loadTextures(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene) {
    loadDiffuseTexture(dir, material, index, scene);
    loadSpecularTexture(dir, material, index, scene);
}

void Mesh::loadDiffuseTexture(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene) {
    if(material->GetTextureCount(aiTextureType_DIFFUSE) > 0){
        aiString path;
        if(material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);
            const aiTexture* texture;
            if((texture = scene->GetEmbeddedTexture(path.C_Str()))){
                std::string hint = texture->achFormatHint;
                if(hint == "png" || hint == "jpg"){
                    if(texture->mHeight == 0) {
                        m_materials[index].diffuse = new Texture(GL_TEXTURE_2D, (u_char *) texture->pcData,
                                                                 texture->mWidth, 3);
                        //m_materials[index].diffuse->load();
                    }
                }
            }else{
                if(p.substr(0, 2) == ".\\") {
                    p = p.substr(2, p.size() - 2);
                }
                std::string fullpath;
                fullpath.append(dir).append("/").append(p);
                m_materials[index].diffuse = new Texture(GL_TEXTURE_2D, fullpath);
            }
        }
    }
}

void Mesh::loadSpecularTexture(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene) {
    if(material->GetTextureCount(aiTextureType_SHININESS) > 0){
        aiString path;
        if(material->GetTexture(aiTextureType_SHININESS, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);
            const aiTexture* texture;
            if((texture = scene->GetEmbeddedTexture(path.C_Str()))){
                std::string hint = texture->achFormatHint;
                if(hint == "png" || hint == "jpg"){
                    if(texture->mHeight == 0){
                        m_materials[index].specularExp = new Texture(GL_TEXTURE_2D, (u_char*)texture->pcData, texture->mWidth, 1);
                    }
                }
            }else {
                if (p.substr(0, 2) == ".\\") {
                    p = p.substr(2, p.size() - 2);
                }
                std::string fullpath;
                fullpath.append(dir).append("/").append(p);
                m_materials[index].specularExp = new Texture(GL_TEXTURE_2D, fullpath);
            }
        }
    }
}

void Mesh::loadColors(const aiMaterial *material, uint32_t index) {
    aiColor3D ambient_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_AMBIENT, ambient_color) == AI_SUCCESS){
        m_materials[index].ambientColor.r = ambient_color.r;
        m_materials[index].ambientColor.g = ambient_color.g;
        m_materials[index].ambientColor.b = ambient_color.b;
    }

    aiColor3D diffuse_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == AI_SUCCESS){
        m_materials[index].diffuseColor.r = diffuse_color.r;
        m_materials[index].diffuseColor.g = diffuse_color.g;
        m_materials[index].diffuseColor.b = diffuse_color.b;
    }

    aiColor3D specular_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == AI_SUCCESS){
        m_materials[index].specularColor.r = specular_color.r;
        m_materials[index].specularColor.g = specular_color.g;
        m_materials[index].specularColor.b = specular_color.b;
    }
}
