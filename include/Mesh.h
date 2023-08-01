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
#include "defs/TextureDefs.h"

#include "Transformation.h"
#include "Texture.h"
#include "Material.h"

#define ASSIMP_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#define INVALID_MATERIAL 0xFFFFFFFF

/**
 * Used to load a mesh from file into the scene.
 * Handles loading of vertices, indices, textures and rendering.
 */
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    /**
     * @brief Loads mesh from a supported file.
     * @param filename File containing a mesh.
     */
    void loadMesh(const std::string& filename);

    /**
     * Draws all the vertices by binding a VAO, binding material textures and calling OpenGL API to draw.
     * @brief Renders the mesh.
     */
    void render();

    /**
     * Returns a reference to mesh transformation that's used to local_position it inside the world.
     * @return Reference to a transformation of the mesh.
     */
    Transformation& transformation();

    Material material();

protected:
    void clear();
    void bufferData();

    enum BUFFER_TYPE{
        INDEX_BUFFER = 0,
        POS_VB       = 1,
        TEXCOORD_VB  = 2,
        NORMAL_VB    = 3,
        WVP_MAT_VB   = 4,
        WORLD_MAT_VB = 5,
        NUM_BUFFERS  = 6
    };

    struct basicMeshEntry {
        uint32_t indicesCount  = 0;
        uint32_t baseVertex    = 0;
        uint32_t baseIndex     = 0;
        uint32_t materialIndex = INVALID_MATERIAL;
    };

    GLuint m_vao = 0;
    GLuint m_buffers[NUM_BUFFERS] = {0};

    std::vector<basicMeshEntry> m_meshes;
    std::vector<Material> m_materials;

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texCoords;
    std::vector<uint32_t>  m_indices;

    Transformation m_transformation;

private:
    void initFromScene(const aiScene* scene, const std::string& filename);
    void initMeshesIndexes(const aiScene *scene);
    void initAllMeshes(const aiScene* scene);
    void initSingleMesh(const aiMesh* mesh);
    void initMaterials(const aiScene* scene, const std::string& filename);
    void loadTextures(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene);
    void loadColors(const aiMaterial* material, uint32_t index);
    void loadDiffuseTexture(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene);
    void loadSpecularTexture(const std::string &dir, const aiMaterial *material, uint32_t index, const aiScene *scene);

    Assimp::Importer m_importer;
};


#endif //TECTONIC_MESH_H
