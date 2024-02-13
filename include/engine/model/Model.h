#ifndef TECTONIC_MODEL_H
#define TECTONIC_MODEL_H

#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <array>
#include <functional>

#include "exceptions.h"
#include "defs/TextureDefs.h"
#include "defs/ShaderDefines.h"

#include "Transformation.h"
#include "Material.h"
#include "utils.h"
#include "engine/model/anim/Animation.h"
#include "engine/model/anim/Bone.h"
#include "ModelTypes.h"
#include "meta/meta.h"

/**
 * Used to load a model from file into the scene.
 * Handles loading of vertices, indices, textures and rendering.
 */
class Model {
    friend class AssimpLoader;
    friend class SkinnedModel;
    friend class EngineCore;
public:
    Model() = default;

    const Material* getMaterial(uint32_t materialIndex) const { return m_materials.size() > materialIndex ? &m_materials.at(materialIndex) : nullptr; }
    const NodeData& getRootNode() const { return m_rootNode; }
    uint32_t getNodeCount() const { return m_nodeCount; }
    GLuint getVAO() const {return m_VAO;};
    uint32_t getMaterialCount() { return m_materials.size(); }
    NodeData* findNode(const std::string& nodeName);

    virtual void bufferMeshes();
    virtual void eraseBuffers();
    virtual void clear();

protected:
    
    enum BUFFER_TYPE{
        INDEX_BUFFER = 0,
        POS_VB       = 1,
        NUM_BUFFERS  = 2
    };

    GLuint m_VAO = -1;
    GLuint m_buffers[NUM_BUFFERS] = {0};

    std::vector<MeshInfo>       m_meshes;
    std::vector<Material>       m_materials;
    std::vector<Vertex>         m_vertices;
    std::vector<uint32_t>       m_indices;

    NodeData m_rootNode;
    uint32_t m_nodeCount = 0;

};


#endif //TECTONIC_MODEL_H
