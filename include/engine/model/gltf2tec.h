#ifndef TECTONIC_GLTF2TEC_H
#define TECTONIC_GLTF2TEC_H

#include <unordered_map>
#include <filesystem>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/detail/type_quat.hpp>
#include <variant>

#include "Logger.h"
#include "utils/Serial.h"

#define GLTF2TEC_VERSION 0

namespace gltf2tec{

    struct GLTFResources {
        using StaticMeshVec_t = std::vector<std::unique_ptr<SerialTypes::Model::MeshAsset<VktTypes::GPU::VertexType::STATIC>>>;
        using SkinnedMeshVec_t = std::vector<std::unique_ptr<SerialTypes::Model::MeshAsset<VktTypes::GPU::VertexType::SKINNED>>>;

        std::variant<StaticMeshVec_t,SkinnedMeshVec_t> meshes;
        std::vector<std::unique_ptr<SerialTypes::Model::Image>> images;
        std::vector<std::unique_ptr<SerialTypes::Model::Node>> nodes;
        std::vector<std::unique_ptr<SerialTypes::Model::GLTFMaterial>> materials;
        std::unique_ptr<SerialTypes::Model::Skin> skin;
        std::vector<std::unique_ptr<SerialTypes::Model::Animation>> animations;
        SerialTypes::Model::NodeID_t topNode = SerialTypes::Model::NULL_ID;
        std::vector<VkSamplerCreateInfo> samplers;
        bool isSkinned = false;
    };

    struct TectonicResources{
        SerialTypes::BinDataVec_t data;
    };

    GLTFResources* loadGltfModel(const std::filesystem::path& filePath);
}


#endif //TECTONIC_GLTF2TEC_H
