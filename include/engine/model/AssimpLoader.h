#ifndef TECTONIC_ASSIMPLOADER_H
#define TECTONIC_ASSIMPLOADER_H

#include <queue>
#include <stack>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>

#include "Model.h"
#include "engine/model/anim/SkinnedModel.h"


#define ASSIMP_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace )

class AssimpLoader {

public:

    AssimpLoader(){
        Assimp::DefaultLogger::create(ASSIMP_DEFAULT_LOG_NAME, Assimp::Logger::VERBOSE);
    }

    std::shared_ptr<Model> loadModel(const std::string& modelFile);
    std::shared_ptr<SkinnedModel> loadSkinnedModel(const std::string& modelFile, const std::string& animationFile = "");

    static glm::mat4 aiMatToGLM(const aiMatrix4x4t<ai_real>& mat);
    static glm::vec3 aiVecToGLM(const aiVector3D& vec);
    static glm::quat aiQuatToGLM(const aiQuaternion& orientation);
    static glm::vec3 aiColToGLM(const aiColor3D& col);

private:

    void loadMaterials(const std::shared_ptr<Model>& model);
    void loadNodes(const std::shared_ptr<Model>& model);
    void loadMeshes(const std::shared_ptr<Model>& model);
    void loadBones(const std::shared_ptr<SkinnedModel>& model);
    void loadAnimations(const std::shared_ptr<SkinnedModel>& model);

    void loadMesh(const aiMesh *mesh, const std::shared_ptr<Model>& model, int32_t& indicesCount, int32_t& verticesCount);
    void loadBoneWeights(const aiMesh* mesh, uint32_t meshIndex, const std::shared_ptr<SkinnedModel>& model);
    static void loadBoneToVertex(Vertex& vertex, int32_t boneID, float weight);
    static void loadMissingBones(const aiAnimation* animation, const std::shared_ptr<SkinnedModel>& model);

    Material createMaterial(const aiMaterial *material);
    std::shared_ptr<Texture> createTexture(const aiMaterial* material, aiTextureType type);
    static Bone createBone(const aiNodeAnim* channel, int32_t boneID);

    std::string m_modelDirectory;
    Assimp::Importer m_importer;
    const aiScene *m_scene = nullptr;
    Assimp::Logger *m_logger = Assimp::DefaultLogger::get();
};


#endif //TECTONIC_ASSIMPLOADER_H
