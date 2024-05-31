#ifndef TECTONIC_VKTLOADER_H
#define TECTONIC_VKTLOADER_H

#include <unordered_map>
#include <filesystem>

#include "engine/vulkan/VktTypes.h"
#include "Logger.h"

struct VktModelResources {
    std::vector<std::unique_ptr<VktTypes::MeshAsset>> meshes;
    std::unordered_map<std::string, VktTypes::MeshAsset*> namedMeshes;

    std::vector<std::unique_ptr<VktTypes::Node>> nodes;
    std::unordered_map<std::string, VktTypes::Node*> namedNodes;

    std::vector<VktTypes::AllocatedImage> images;
    std::unordered_map<std::string, VktTypes::AllocatedImage*> namedImages;

    std::vector<std::unique_ptr<VktTypes::GLTFMaterial>> materials;
    std::unordered_map<std::string, VktTypes::GLTFMaterial*> namedMaterials;

    std::unique_ptr<VktTypes::Skin> skin;

    std::vector<std::unique_ptr<VktTypes::Animation>> animations;
    std::unordered_map<std::string, VktTypes::Animation*> namedAnimations;

    std::vector<VktTypes::Node*> topNodes;
    std::vector<VkSampler> samplers;
    DescriptorAllocatorDynamic descriptorPool;
    VktTypes::AllocatedBuffer materialDataBuffer;

    ~VktModelResources(){ clean(); }
    void gatherContext(const glm::mat4& topMatrix, const VktTypes::GPUJointsBuffers& jointsBuffers, VktTypes::DrawContext& ctx);
private:
    void clean();
};

VktModelResources* loadGltfModel(const std::filesystem::path& filePath);

#endif //TECTONIC_VKTLOADER_H
