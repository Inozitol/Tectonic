#ifndef TECTONIC_MESH_H
#define TECTONIC_MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <glm/mat4x4.hpp>
#include <vector>
#include <tuple>
#include "extern/glad/glad.h"

#include "exceptions.h"
#include "texture_defines.h"

#include "Transformation.h"
#include "Texture.h"
#include "Material.h"

#define ASSIMP_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#define INVALID_MATERIAL 0xFFFFFFFF

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    void load_mesh(const std::string& filename);
    void render();
    Transformation& transformation();
    Material material();

private:
    void clear();
    void init_from_scene(const aiScene* scene, const std::string& filename);
    void init_meshes_indexes(const aiScene *scene);
    void init_all_meshes(const aiScene* scene);
    void init_single_mesh(const aiMesh* mesh);
    void init_materials(const aiScene* scene, const std::string& filename);
    void load_textures(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene);
    void load_colors(const aiMaterial* material, uint32_t index);
    void load_diffuse_texture(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene);
    void
    load_specular_texture(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene);
    void buffer_data();

    enum BUFFER_TYPE{
        INDEX_BUFFER = 0,
        POS_VB       = 1,
        TEXCOORD_VB  = 2,
        NORMAL_VB    = 3,
        WVP_MAT_VB   = 4,
        WORLD_MAT_VB = 5,
        NUM_BUFFERS  = 6
    };

    GLuint _vao = 0;
    GLuint _buffers[NUM_BUFFERS] = {0};

    struct basic_mesh_entry{
        uint32_t indices_count  = 0;
        uint32_t base_vertex    = 0;
        uint32_t base_index     = 0;
        uint32_t material_index = INVALID_MATERIAL;
    };

    std::vector<basic_mesh_entry> _meshes;
    std::vector<Material> _materials;

    std::vector<glm::vec3> _positions;
    std::vector<glm::vec3> _normals;
    std::vector<glm::vec2> _tex_coords;
    std::vector<uint32_t>  _indices;

    Assimp::Importer _importer;

    Transformation _transformation;
};


#endif //TECTONIC_MESH_H
