#include <iostream>
#include "Mesh.h"
#include "utils.h"

#define POSITION_LOCATION   0
#define TEX_COORD_LOCATION  1
#define NORMAL_LOCATION     2

Mesh::~Mesh() {
    clear();
}

void Mesh::load_mesh(const std::string &filename) {
    // Clear previous mesh
    clear();

    // Create VAO
    glGenVertexArrays(1, &_vao);
    EmpheralBindVAO bind(_vao);

    // Create buffers for vertices
    glGenBuffers(ARRAY_SIZE(_buffers), _buffers);

    const aiScene* scene = _importer.ReadFile(filename.c_str(), ASSIMP_FLAGS);
    if(scene){
        init_from_scene(scene, filename);
    }else{
        throw mesh_exception("Unable to parse file");
    }
}

void Mesh::init_from_scene(const aiScene *scene, const std::string &filename) {
    _meshes.resize(scene->mNumMeshes);
    _materials.resize(scene->mNumMaterials);

    init_meshes_indexes(scene);
    init_all_meshes(scene);
    init_materials(scene, filename);

    buffer_data();
}

void Mesh::clear(){
    {
        EmpheralBindVAO bind(_vao);

        // Delete OpenGL buffers
        glDeleteBuffers(1, &_buffers[POS_VB]);
        glDeleteBuffers(1, &_buffers[TEXCOORD_VB]);
        glDeleteBuffers(1, &_buffers[NORMAL_VB]);
        glDeleteBuffers(1, &_buffers[INDEX_BUFFER]);
    }

    // Delete VAO
    glDeleteVertexArrays(1, &_vao);
}

void Mesh::init_meshes_indexes(const aiScene *scene) {
    uint32_t c_vertices = 0, c_indices = 0;

    // Init indexes to vertices and indices
    for(uint32_t i = 0; i < _meshes.size(); i++){
        _meshes[i].material_index = scene->mMeshes[i]->mMaterialIndex;
        _meshes[i].indices_count  = scene->mMeshes[i]->mNumFaces*3;
        _meshes[i].base_vertex    = c_vertices;
        _meshes[i].base_index     = c_indices;

        c_vertices += scene->mMeshes[i]->mNumVertices;
        c_indices  += _meshes[i].indices_count;
    }
}

void Mesh::init_all_meshes(const aiScene *scene) {
    for(uint32_t i = 0; i < _meshes.size(); i++){
        const aiMesh* mesh = scene->mMeshes[i];
        init_single_mesh(mesh);
    }
}

void Mesh::init_single_mesh(const aiMesh *mesh) {
    const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

    // Init vertices
    for(uint32_t i = 0; i < mesh->mNumVertices; i++){
        const aiVector3D& pos       = mesh->mVertices[i];
        const aiVector3D& normal    = mesh->mNormals[i];
        const aiVector3D& tex_coord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero3D;

        _positions.emplace_back(pos.x, pos.y, pos.z);
        _normals.emplace_back(normal.x, normal.y, normal.z);
        _tex_coords.emplace_back(tex_coord.x, tex_coord.y);
    }

    // Init indices
    for(uint32_t i = 0; i < mesh->mNumFaces; i++){
        const aiFace& face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        _indices.emplace_back(face.mIndices[0]);
        _indices.emplace_back(face.mIndices[1]);
        _indices.emplace_back(face.mIndices[2]);
    }
}

void Mesh::init_materials(const aiScene *scene, const std::string &filename) {
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
        load_textures(dir, material, i);
        load_colors(material, i);
    }
}

void Mesh::buffer_data() {
    // Buffers for vertices
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(_positions[0]) * _positions.size()), &_positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Buffers for texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(_tex_coords[0]) * _tex_coords.size()), &_tex_coords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Buffers for normals
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(_normals[0]) * _normals.size()), &_normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Buffers for indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(_indices[0]) * _indices.size()), &_indices[0], GL_STATIC_DRAW);
}

void Mesh::render() {
    // Binding VAO for render
    EmpheralBindVAO bind(_vao);
    for(const auto& mesh : _meshes){
        uint32_t material_index = mesh.material_index;

        assert(material_index < _materials.size());

        if(_materials[material_index].diffuse){
            _materials[material_index].diffuse->bind(COLOR_TEXTURE_UNIT);
        }
        if(_materials[material_index].specular_exponent){
            _materials[material_index].specular_exponent->bind(SPECULAR_EXPONENT_UNIT);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES,
                                 static_cast<GLsizei>(mesh.indices_count),
                                 GL_UNSIGNED_INT,
                                 (void*)(sizeof(mesh.base_index) * mesh.base_index),
                                 static_cast<GLsizei>(mesh.base_vertex));
    }
}

Transformation& Mesh::transformation() {
    return _transformation;
}

const Material Mesh::material() {
    for(const auto& material : _materials){
        if(material.ambient_color != glm::vec3(0.0f, 0.0f, 0.0f)){
            return material;
        }
    }
    return Material({glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f)});
}

void Mesh::load_textures(const std::string &dir, const aiMaterial *material, uint32_t index) {
    load_diffuse_texture(dir, material, index);
    load_specular_texture(dir, material, index);
}

void Mesh::load_diffuse_texture(const std::string &dir, const aiMaterial *material, uint32_t index) {
    if(material->GetTextureCount(aiTextureType_DIFFUSE) > 0){
        aiString path;
        if(material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);

            if(p.substr(0, 2) == ".\\"){
                p = p.substr(2, p.size() - 2);
            }

            std::string fullpath;
            fullpath.append(dir).append("/").append(p);
            _materials[index].diffuse = new Texture(GL_TEXTURE_2D, fullpath);
            _materials[index].diffuse->load();
        }
    }
}

void Mesh::load_specular_texture(const std::string &dir, const aiMaterial *material, uint32_t index) {
    if(material->GetTextureCount(aiTextureType_SHININESS) > 0){
        aiString path;
        if(material->GetTexture(aiTextureType_SHININESS, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS){
            std::string p(path.data);

            if(p.substr(0, 2) == ".\\"){
                p = p.substr(2, p.size() - 2);
            }

            std::string fullpath;
            fullpath.append(dir).append("/").append(p);
            _materials[index].specular_exponent = new Texture(GL_TEXTURE_2D, fullpath);
            _materials[index].specular_exponent->load();
        }
    }
}

void Mesh::load_colors(const aiMaterial *material, uint32_t index) {
    aiColor3D ambient_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_AMBIENT, ambient_color) == AI_SUCCESS){
        printf("Loaded ambient color [%f | %f | %f]\n", ambient_color.r, ambient_color.g, ambient_color.b);
        _materials[index].ambient_color.r = ambient_color.r;
        _materials[index].ambient_color.g = ambient_color.g;
        _materials[index].ambient_color.b = ambient_color.b;
    }

    aiColor3D diffuse_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == AI_SUCCESS){
        printf("Loaded diffuse color [%f | %f | %f]\n", diffuse_color.r, diffuse_color.g, diffuse_color.b);
        _materials[index].diffuse_color.r = diffuse_color.r;
        _materials[index].diffuse_color.g = diffuse_color.g;
        _materials[index].diffuse_color.b = diffuse_color.b;
    }

    aiColor3D specular_color(0.0f, 0.0f, 0.0f);
    if(material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == AI_SUCCESS){
        printf("Loaded specular color [%f | %f | %f]\n", specular_color.r, specular_color.g, specular_color.b);
        _materials[index].specular_color.r = specular_color.r;
        _materials[index].specular_color.g = specular_color.g;
        _materials[index].specular_color.b = specular_color.b;
    }
}
