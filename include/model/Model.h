#ifndef TECTONIC_MODEL_H
#define TECTONIC_MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <array>
#include <functional>
#include "extern/glad/glad.h"

#include "exceptions.h"
#include "defs/TextureDefs.h"
#include "defs/ShaderDefines.h"

#include "Transformation.h"
#include "Texture.h"
#include "Material.h"
#include "utils.h"
#include "shader/LightingShader.h"
#include "Animation.h"
#include "Bone.h"
#include "ModelTypes.h"
#include "meta/meta.h"

#define ASSIMP_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace)

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

    GLuint getVAO() const {return m_VAO;};

    const Material* getMaterial(uint32_t materialIndex) const { return m_materials.size() > materialIndex ? &m_materials.at(materialIndex) : nullptr; }
    const BoneInfo* getBoneInfo(const std::string& boneName) const {
        if(m_boneInfoMap.contains(boneName))
            return &m_boneInfoMap.at(boneName);
        else
            return nullptr;
    }
    const NodeData& getRootNode() const { return m_rootNode; }

    Animation* getAnimation(uint32_t animIndex) {
        if(animIndex < m_animations.size())
            return &m_animations.at(animIndex);
        else
            return nullptr;
    }

    std::vector<Mesh>       m_meshes;
    std::vector<Material>   m_materials;

protected:

    std::vector<Vertex>     m_vertices;
    std::vector<uint32_t>   m_indices;

    enum BUFFER_TYPE{
        INDEX_BUFFER = 0,
        POS_VB       = 1,
        NUM_BUFFERS  = 2
    };

    GLuint m_VAO = -1;
    GLuint m_buffers[NUM_BUFFERS] = {0};

    uint32_t m_indicesCount = 0;
    uint32_t m_verticesCount = 0;

    void bufferMeshes();

private:
    std::unordered_map<std::string, BoneInfo> m_boneInfoMap;
    int32_t m_boneCounter = 0;
    std::vector<Animation> m_animations;
    uint32_t m_currentAnim = -1;
    double m_currentAnimTime = 0.0;
    std::array<glm::mat4, MAX_BONES> m_finalBoneMatrices{};

    NodeData m_rootNode;

    std::string m_modelDirectory;

    int32_t getBoneId(const aiBone* bone);

    void loadModel(const std::string& filename);
    void loadMaterials(const aiScene* scene);
    void loadAnimations(const aiScene* scene);
    void processNode(const aiNode *node, const aiScene *scene, NodeData &nodeDest);
    void addMesh(aiMesh* mesh, const aiScene* scene);

    void setVertexBoneData(Vertex& vertex, int32_t boneId, float weight);
    void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);

    void readMissingBoneData(const aiAnimation* animation);
    void calcBoneTransform(const NodeData* node, glm::mat4 parentTransform);
};


#endif //TECTONIC_MODEL_H
