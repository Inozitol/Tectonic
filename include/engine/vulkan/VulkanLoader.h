#ifndef TECTONIC_VULKANLOADER_H
#define TECTONIC_VULKANLOADER_H

#include <unordered_map>
#include <filesystem>

#include "engine/vulkan/VulkanTypes.h"

struct GeoSurface{
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset{
    std::string name;
    std::vector<GeoSurface> surfaces;
    VkTypes::GPUMeshBuffers meshBuffers;
};

class VulkanCore;

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanCore* core, std::filesystem::path filePath);

#endif //TECTONIC_VULKANLOADER_H
