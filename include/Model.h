#ifndef TECTONIC_MODEL_H
#define TECTONIC_MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <glm/mat4x4.hpp>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <array>
#include "extern/glad/glad.h"

#include "exceptions.h"
#include "defs/TextureDefs.h"
#include "defs/ShaderDefines.h"

#include "Transformation.h"
#include "Texture.h"
#include "Material.h"
#include "utils.h"

#define ASSIMP_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#define INVALID_MATERIAL 0xFFFFFFFF
#define MAX_NUM_BONES_PER_VERTEX 4

struct Vertex{
    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 m_normal = {0.0f, 0.0f, 0.0f};
    glm::vec2 m_texCoord = {0.0f, 0.0f};
    glm::vec3 m_tangent;
    glm::vec3 m_bitangent;

    int m_boneIds[MAX_BONES_INFLUENCE];
    float m_weights[MAX_BONES_INFLUENCE];
};

class Mesh{
public:
    std::vector<Vertex>     m_vertices;
    std::vector<uint32_t>   m_indices;
    Material                m_material;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const Material& material);
    void render() const;
private:
    GLuint m_VAO, m_VBO, m_EBO;
    void initMesh();
};

/**
 * Used to load a model from file into the scene.
 * Handles loading of vertices, indices, textures and rendering.
 */
class Model {
public:
    Model() = default;

    /**
     * @brief Loads model from a supported file.
     * @param filename File containing a model.
     */
    void loadModelFromFile(const std::string& filename);

    /**
     * Draws all the vertices by binding a VAO, binding material textures and calling OpenGL API to draw.
     * @brief Renders the model.
     */
    void render() const;

    Material material();

protected:

    std::vector<Mesh> m_meshes;
    std::vector<Material> m_materials;

private:
    std::string m_modelDirectory;

    void loadModel(const std::string& filename);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    void loadTextures(const aiMaterial *aiMat, Material& material, const aiScene *scene);
    void loadDiffuseTexture(const aiMaterial *aiMat, Material& material, const aiScene *scene);
    void loadSpecularTexture(const aiMaterial *aiMat, Material& material, const aiScene *scene);
    void loadColors(const aiMaterial* aiMat, Material& material);

};


#endif //TECTONIC_MODEL_H
